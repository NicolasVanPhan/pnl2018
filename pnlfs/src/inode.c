
#include "pnlfs.h"

/*
int	pnlfs_dir_rm(struct inode *dir, ino_t ino);
int	pnlfs_dir_add(struct inode *dir, const char *name, ino_t ino);
int	pnlfs_dir_set_name(struct inode *dir, ino_t ino, char *src);
int	pnlfs_dir_get_name(struct inode *dir, ino_t ino, char *dest);
ino_t	pnlfs_dir_get_ino(struct inode *dir, const char *name);
int	pnlfs_write_inode_state(struct super_block *sb, ino_t ino, char val);
int	pnlfs_read_inode_state(struct super_block *sb, ino_t ino);
ino_t	pnlfs_get_first_free_ino(struct super_block *sb);
int	pnlfs_write_inode(struct super_block, struct pnlfs_inode, ino_t);
struct pnlfs_inode
	*pnlfs_read_inode(struct super_block *sb, ino_t ino);
*/

/* -------------------------------------------------------------------------- */
/* ---------- DATA : DIRECTORIES -------------------------------------------- */
/* -------------------------------------------------------------------------- */

int	pnlfs_dir_rm(struct inode *dir, ino_t ino)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (dir == NULL) {
		pr_err("pnlfs_dir_rm() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the entry matching our ino */
	ret = -1;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (le32_to_cpu(rows[i].inode) == ino) {
			strncpy(rows[i].filename, "", PNLFS_FILENAME_LEN);
			rows[i].inode = 0;
			ret = 0;
			pnli->nr_entries--;
			mark_inode_dirty(&pnli->vfs_inode);
			mark_buffer_dirty(bh);
			break;
		}
	}
	brelse(bh);
	return ret;
}

int	pnlfs_dir_add(struct inode *dir, const char *name, ino_t ino)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;

	if (name == NULL) {
		pr_err("pnlfs_dir_add() : NULL parameter 'name'\n");
		return -EINVAL;
	}
	if (name[0] == '\0') {
		pr_err("pnlfs_dir_add() : Empty name\n");
		return -EINVAL;
	}
	if (strnlen(name, PNLFS_FILENAME_LEN) == PNLFS_FILENAME_LEN) {
		pr_err("Filename too long\n");
		return -EINVAL;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_add() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);
	rows = (struct pnlfs_file *)bh->b_data;

	/* If the directory is full, quit */
	if (nr_entries == PNLFS_MAX_DIR_ENTRIES) {
		pr_err("pnlfs_dir_add() : Directory is full\n");
		return -ENOMEM;
	}

	/* Find a free row where we could add our entry */
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // found a free entry
			break;
	}

	/* If the directory is full despite nr_entries < max (shouldnt happen)*/
	if (i == PNLFS_MAX_DIR_ENTRIES) {
		pr_err("pnlfs_dir_add() : No free entries, image corrupted\n");
		return -ENOMEM;
	}

	/* Add our entry */
	rows[i].inode = cpu_to_le32(ino);
	strncpy(rows[i].filename, name, PNLFS_FILENAME_LEN);
	pnli->nr_entries++;
	mark_inode_dirty(&pnli->vfs_inode);
	mark_buffer_dirty(bh);
	brelse(bh);

	return 0;
}

int	pnlfs_dir_set_name(struct inode *dir, ino_t ino, char *src)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (src == NULL) {
		pr_err("pnlfs_dir_set_name() : NULL parameter 'name'\n");
		return -EINVAL;
	}
	if (src[0] == '\0') {
		pr_err("pnlfs_dir_set_name() : Empty name\n");
		return -EINVAL;
	}
	if (strnlen(src, PNLFS_FILENAME_LEN) == PNLFS_FILENAME_LEN) {
		pr_err("Filename too long\n");
		return -EINVAL;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_set_name() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the filename matching our ino */
	ret = -1;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (le32_to_cpu(rows[i].inode) == ino) {
			strncpy(rows[i].filename, src, PNLFS_FILENAME_LEN);
			ret = 0;
			mark_buffer_dirty(bh);
			break;
		}
	}
	brelse(bh);
	return ret;
}

int	pnlfs_dir_get_name(struct inode *dir, ino_t ino, char *dest)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (dest == NULL) {
		pr_err("pnlfs_dir_get_name() : NULL parameter 'name'\n");
		return -EINVAL;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_get_name() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the filename matching our ino */
	ret = -1;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (le32_to_cpu(rows[i].inode) == ino) {
			strncpy(dest, rows[i].filename, PNLFS_FILENAME_LEN);
			ret = 0;
			break;
		}
	}
	brelse(bh);
	return ret;
}

ino_t	pnlfs_dir_get_ino(struct inode *dir, const char *name)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (name == NULL) {
		pr_err("pnlfs_dir_get_ino() : NULL parameter 'name'\n");
		return INO_ERR;
	}
	if (name[0] == '\0') {
		pr_err("pnlfs_dir_get_ino() : Empty name\n");
		return INO_ERR;
	}
	if (strnlen(name, PNLFS_FILENAME_LEN) == PNLFS_FILENAME_LEN) {
		pr_err("Filename too long\n");
		return INO_ERR;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_get_ino() : NULL parameter 'dir'\n");
		return INO_ERR;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the ino matching our filename */
	ret = INO_UNFOUND;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (!strncmp(rows[i].filename, name, PNLFS_FILENAME_LEN)) {
			ret = le32_to_cpu(rows[i].inode);
			break;
		}
	}
	brelse(bh);
	return ret;
}

/* -------------------------------------------------------------------------- */
/* ---------- BFREE --------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/*
 * Sets the bit corresponding to block 'bno' in the bfree bitmap
 */
int pnlfs_write_block_state(struct super_block *sb, sector_t bno, char val)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkfirst;
	sector_t		blkno;
	int			row;		// row nb relative to cur blk
	int			absrow;		// row nb absolute to first blk
	int			nb_rows_per_block;
	int			bitno;
	struct buffer_head	*bh;
	int			oldval;
	int			rowval;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (bno < 0 || bno >= psbi->nr_blocks)
		return -EINVAL;

	/* Determine the exact location of the bit indicating the 'bno' state */
	blkfirst = 1 + psbi->nr_istore_blocks + psbi->nr_ifree_blocks;
	absrow = bno / 32;
	nb_rows_per_block = PNLFS_BLOCK_SIZE / 4;
	blkno = blkfirst + absrow / nb_rows_per_block;
	row = absrow % nb_rows_per_block;
	bitno = bno % 32;

	/* Writing the block state */
	bh = sb_bread(sb, blkno);
	rowval = le32_to_cpu(((unsigned int *)bh->b_data)[row]);
	oldval = rowval & (1 << bitno);
	rowval &= ~(1 << bitno);
	if (val)
		rowval |= (1 << bitno);
	// Update superblock
	if (val && !oldval)
		psbi->nr_free_blocks++;
	if (!val && oldval)
		psbi->nr_free_blocks--;
	((unsigned int *)bh->b_data)[row] = le32_to_cpu(rowval);
	brelse(bh);
	return 0;
}

/*
 * Returns the bit corresponding to block 'bno' in the bfree bitmap
 */
int pnlfs_read_block_state(struct super_block *sb, sector_t bno)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkfirst;
	sector_t		blkno;
	int			row;		// row nb relative to cur blk
	int			absrow;		// row nb absolute to first blk
	int			nb_rows_per_block;
	int			bitno;
	struct buffer_head	*bh;
	int			rowval;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (bno < 0 || bno >= psbi->nr_blocks)
		return -EINVAL;

	/* Determine the exact location of the bit indicating the 'bno' state */
	blkfirst = 1 + psbi->nr_istore_blocks + psbi->nr_ifree_blocks;
	absrow = bno / 32;
	nb_rows_per_block = PNLFS_BLOCK_SIZE / 4;
	blkno = blkfirst + absrow / nb_rows_per_block;
	row = absrow % nb_rows_per_block;
	bitno = bno % 32;

	/* Reading the inode state */
	bh = sb_bread(sb, blkno);
	rowval = le32_to_cpu(((unsigned int *)bh->b_data)[row]);
	brelse(bh);
	if (rowval & (1<<bitno))
		return 1;
	else
		return 0;
}

/*
 * This function gets the bno of the first free block on the disk
 */
ino_t pnlfs_get_first_free_bno(struct super_block *sb)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		bno;
	sector_t		blkno;
	sector_t		blkfirst;
	sector_t		blklast;
	struct buffer_head	*bh;
	unsigned long		nb_rows_per_block; // how many rows in a blk
	unsigned long		nb_total_rows;  // how many rows in total
	unsigned long		nb_rows;	// how many rows in current blk
	unsigned long		*addr;
	int			nb_blocks_full;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	blkfirst = 1 + psbi->nr_istore_blocks + psbi->nr_ifree_blocks;
	blklast = blkfirst + psbi->nr_bfree_blocks;
	nb_rows_per_block = PNLFS_BLOCK_SIZE / sizeof(unsigned long);
	nb_total_rows = psbi->nr_blocks / sizeof(unsigned long);

	/* For each bfree block */
	nb_blocks_full = 0;
	for (blkno = blkfirst; blkno < blklast; blkno++) {
		/* Read the current bfree block */
		bh = sb_bread(sb, blkno);
		addr = (unsigned long *)bh->b_data;

		/* Get how many rows we have in the current block */
		nb_rows = nb_rows_per_block;
		if (blkno == blklast - 1) // Last bfree block may not be full */
			nb_rows = nb_total_rows % nb_rows_per_block;

		/* Look for the first free block */
		bno = find_first_bit(addr, nb_rows * 64);
		brelse(bh);

		/* If we found a free inode, bingo */
		if (bno < nb_rows * 64)
			return (nb_blocks_full * nb_total_rows * 64) + bno;

		nb_blocks_full++;
	}
	pr_err("Disk full : no blocks available\n");
	return -1;
}



/* -------------------------------------------------------------------------- */
/* ---------- IFREE --------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/*
 * Sets the bit corresponding to inode 'ino' in the ifree bitmap
 */
int pnlfs_write_inode_state(struct super_block *sb, ino_t ino, char val)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkfirst;
	sector_t		blkno;
	int			row;		// row nb relative to cur blk
	int			absrow;		// row nb absolute to first blk
	int			nb_rows_per_block;
	int			bitno;
	struct buffer_head	*bh;
	int			rowval;
	int			oldval;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (ino >= psbi->nr_inodes)
		return -EINVAL;

	/* Determine the exact location of the bit indicating the 'ino' state */
	blkfirst = 1 + psbi->nr_istore_blocks;
	absrow = ino / 32;
	nb_rows_per_block = PNLFS_BLOCK_SIZE / 4;
	blkno = blkfirst + absrow / nb_rows_per_block;
	row = absrow % nb_rows_per_block;
	bitno = ino % 32;

	/* Writing the inode state */
	bh = sb_bread(sb, blkno);
	rowval = le32_to_cpu(((unsigned int *)bh->b_data)[row]);
	oldval = rowval & (1 << bitno);
	rowval &= ~(1 << bitno);
	if (val)
		rowval |= (1 << bitno);
	// Update superblock
	if (val && !oldval)
		psbi->nr_free_inodes++;
	if (!val && oldval)
		psbi->nr_free_inodes--;
	((unsigned int *)bh->b_data)[row] = le32_to_cpu(rowval);
	brelse(bh);
	return 0;
}

/*
 * Returns the bit corresponding to inode 'ino' in the ifree bitmap
 */
int pnlfs_read_inode_state(struct super_block *sb, ino_t ino)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkfirst;
	sector_t		blkno;
	int			row;		// row nb relative to cur blk
	int			absrow;		// row nb absolute to first blk
	int			nb_rows_per_block;
	int			bitno;
	struct buffer_head	*bh;
	int			rowval;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (ino >= psbi->nr_inodes)
		return -EINVAL;

	/* Determine the exact location of the bit indicating the 'ino' state */
	blkfirst = 1 + psbi->nr_istore_blocks;
	absrow = ino / 32;
	nb_rows_per_block = PNLFS_BLOCK_SIZE / 4;
	blkno = blkfirst + absrow / nb_rows_per_block;
	row = absrow % nb_rows_per_block;
	bitno = ino % 32;

	/* Reading the inode state */
	bh = sb_bread(sb, blkno);
	rowval = le32_to_cpu(((unsigned int *)bh->b_data)[row]);
	brelse(bh);
	if (rowval & (1<<bitno))
		return 1;
	else
		return 0;
}

/*
 * This function gets the ino of the first free inode on the disk
 */
ino_t pnlfs_get_first_free_ino(struct super_block *sb)
{
	struct pnlfs_sb_info	*psbi;
	ino_t			ino;
	sector_t		blkno;
	sector_t		blkfirst;
	sector_t		blklast;
	struct buffer_head	*bh;
	unsigned long		nb_rows_per_block; // how many rows in a blk
	unsigned long		nb_total_rows;  // how many rows in total
	unsigned long		nb_rows;	// how many rows in current blk
	unsigned long		*addr;
	int			nb_blocks_full;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	blkfirst = 1 + psbi->nr_istore_blocks;
	blklast = blkfirst + psbi->nr_bfree_blocks;
	nb_rows_per_block = PNLFS_BLOCK_SIZE / sizeof(unsigned long);
	nb_total_rows = psbi->nr_inodes / sizeof(unsigned long);

	/* For each ifree block */
	nb_blocks_full = 0;
	for (blkno = blkfirst; blkno < blklast; blkno++) {
		/* Read the current ifree block */
		bh = sb_bread(sb, blkno);
		addr = (unsigned long *)bh->b_data;

		/* Get how many rows we have in the current block */
		nb_rows = nb_rows_per_block;
		if (blkno == blklast - 1) // Last ifree block may not be full */
			nb_rows = nb_total_rows % nb_rows_per_block;

		/* Look for the first free inode */
		ino = find_first_bit(addr, nb_rows * 64);
		brelse(bh);

		/* If we found a free inode, bingo */
		if (ino < nb_rows * 64)
			return (nb_blocks_full * nb_total_rows * 64) + ino;

		nb_blocks_full++;
	}
	pr_err("Disk full : no inodes available\n");
	return -1;
}

/* -------------------------------------------------------------------------- */
/* ---------- ISTORE -------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/*
 * This function writes a pnlfs inode on a pnlfs image
 */
int pnlfs_write_inode(struct super_block *sb,				 // [DONE]
	struct pnlfs_inode *pnli ,ino_t ino)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkno;
	struct buffer_head	*bh;
	int			row; // inode row in disk block

	if (pnli == NULL) {
		pr_err("pnlfs_write_inode() : Invalid NULL inode\n");
		return -EINVAL;
	}
	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (ino < 0 || ino >= psbi->nr_inodes) {
		pr_err("pnlfs_write_inode() : Inode out of range\n");
		return -EINVAL;
	}
	blkno = 1 + ino / PNLFS_INODES_PER_BLOCK;
	bh = sb_bread(sb,  blkno);
	if (bh == NULL) {
		brelse(bh);
		pr_err("pnlfs_write_inode() : Block %ld read failed\n", blkno);
		return -EIO;
	}
	row = ino % PNLFS_INODES_PER_BLOCK;
	((struct pnlfs_inode *)bh->b_data)[row] = *pnli;
	mark_buffer_dirty(bh);
	brelse(bh);
	return 0;
}

/*
 * This function reads a pnlfs inode on a pnlfs image
 */
struct pnlfs_inode *pnlfs_read_inode(struct super_block *sb, ino_t ino)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkno;
	struct buffer_head	*bh;
	struct pnlfs_inode	*pnli;
	int			row; // inode row in disk block

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (ino < 0 || ino >= psbi->nr_inodes) {
		pr_err("pnlfs_read_inode() : Inode out of range\n");
		return ERR_PTR(-EINVAL);
	}
	blkno = 1 + ino / PNLFS_INODES_PER_BLOCK;
	bh = sb_bread(sb,  blkno);
	if (bh == NULL) {
		brelse(bh);
		pr_err("pnlfs_read_inode() : Block %ld read failed\n", blkno);
		return ERR_PTR(-EIO);
	}
	row = ino % PNLFS_INODES_PER_BLOCK;
	pnli = kmalloc(sizeof(struct pnlfs_inode), GFP_KERNEL);
	if (pnli == NULL) {
		brelse(bh);
		pr_err("pnlfs_read_inode() : Kmalloc failed\n");
		return ERR_PTR(-ENOMEM);
	}
	*pnli = ((struct pnlfs_inode *)bh->b_data)[row];
	brelse(bh);
	return pnli;
}


