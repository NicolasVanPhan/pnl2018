
#include "pnlfs.h"

/*
ino_t	pnlfs_get_ino_from_name(struct inode *dir, const char *name);
void	pnlfs_write_inode_state(struct super_block *sb, ino_t ino, char val);
int	pnlfs_read_inode_state(struct super_block *sb, ino_t ino);
ino_t	pnlfs_get_first_free_ino(struct super_block *sb);
int	pnlfs_write_inode(struct super_block, struct pnlfs_inode, ino_t);
struct pnlfs_inode
	*pnlfs_read_inode(struct super_block *sb, ino_t ino);
*/

ino_t	pnlfs_get_ino_from_name(struct inode *dir, const char *name)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (strnlen(name, PNLFS_FILENAME_LEN) == PNLFS_FILENAME_LEN) {
		pr_err("Filename too long\n");
		return INO_ERR;
	}

	if (dir == NULL)
		return INO_ERR;

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the ino matching our filename */
	ret = INO_ERR;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < nr_entries; i++) {
		if (!strncmp(rows[i].filename, name, PNLFS_FILENAME_LEN)) {
			ret = le32_to_cpu(rows[i].inode);
			break;
		}
	}
	brelse(bh);
	return ret;
}

/*
 * Sets the bit corresponding to inode 'ino' in the ifree bitmap
 */
void pnlfs_write_inode_state(struct super_block *sb, ino_t ino, char val)	 // [DONE]
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkfirst;
	sector_t		blkno;
	ulong			byteno;
	int			bitno;
	char			*byte;
	struct buffer_head	*bh;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	blkfirst = 1 + psbi->nr_istore_blocks;
	blkno = blkfirst + (ino / sizeof(char)) / PNLFS_BLOCK_SIZE;
	byteno = (ino / sizeof(char)) % PNLFS_BLOCK_SIZE;
	bitno = ino % sizeof(char);

	bh = sb_bread(sb, blkno);
	byte = &((char *)bh->b_data)[byteno];
	*byte &= ~(1 << bitno);
	if (val)
		*byte |= (1 << bitno);
	mark_buffer_dirty(bh);
	brelse(bh);
}

/*
 * Returns the bit corresponding to inode 'ino' in the ifree bitmap
 */
int pnlfs_read_inode_state(struct super_block *sb, ino_t ino)		 // [DONE]
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkfirst;
	sector_t		blkno;
	ulong			byteno;
	int			bitno;
	struct buffer_head	*bh;
	char			val;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	blkfirst = 1 + psbi->nr_istore_blocks;
	blkno = blkfirst + (ino / sizeof(char)) / PNLFS_BLOCK_SIZE;
	byteno = (ino / sizeof(char)) % PNLFS_BLOCK_SIZE;
	bitno = ino % sizeof(char);

	bh = sb_bread(sb, blkno);
	val = ((char *)bh->b_data)[byteno] & bitno;
	brelse(bh);
	return val;
}

/*
 * This function gets the ino of the first free inode on the disk
 */
ino_t pnlfs_get_first_free_ino(struct super_block *sb)				 // [DONE]
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

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	blkfirst = 1 + psbi->nr_istore_blocks;
	blklast= 1 + psbi->nr_istore_blocks + psbi->nr_ifree_blocks;
	nb_rows_per_block = PNLFS_BLOCK_SIZE / sizeof(unsigned long);
	nb_total_rows = psbi->nr_inodes / sizeof(unsigned long);

	/* For each ifree block */
	for (blkno = blkfirst; blkno < blklast; blkno++) {
		/* Read the current ifree block */
		bh = sb_bread(sb, blkno);
		addr = (unsigned long *)bh->b_data;

		/* Get how many rows we have in the current block */
		nb_rows = nb_rows_per_block;
		if (blkno == blklast - 1) // Last ifree block may not be full */
			nb_rows = nb_total_rows % nb_rows_per_block;

		/* Look for the first free inode */
		ino = find_first_bit(addr, nb_rows);
		brelse(bh);

		/* If we found a free inode, bingo */
		if (ino < nb_rows)
			return ino;

	}
	pr_warn("Disk full : no inode available\n");
	return -1;
}

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

	if (pnli == NULL)
		return -EINVAL;
	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (ino < 0 || ino >= psbi->nr_inodes)
		return -EINVAL;
	blkno = 1 + ino / PNLFS_INODES_PER_BLOCK;
	bh = sb_bread(sb,  blkno);
	if (bh == NULL)
		return -EIO;
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
		pr_err("pnlfs_read_inode() : Block %ld read failed\n", blkno);
		return ERR_PTR(-EIO);
	}
	row = ino % PNLFS_INODES_PER_BLOCK;
	pnli = kmalloc(sizeof(struct pnlfs_inode), GFP_KERNEL);
	if (pnli == NULL) {
		pr_err("pnlfs_read_inode() : Kmalloc failed\n");
		return ERR_PTR(-ENOMEM);
	}
	*pnli = ((struct pnlfs_inode *)bh->b_data)[row];
	brelse(bh);
	return pnli;
}


