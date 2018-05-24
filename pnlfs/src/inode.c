
#include "pnlfs.h"

int	pnlfs_write_block_state(struct super_block *sb, sector_t bno, char val);
int	pnlfs_read_block_state(struct super_block *sb, sector_t bno);
long	pnlfs_get_first_free_bno(struct super_block *sb);
int	pnlfs_write_inode_state(struct super_block *sb, ino_t ino, char val);
int	pnlfs_read_inode_state(struct super_block *sb, ino_t ino);
long	pnlfs_get_first_free_ino(struct super_block *sb);
int	pnlfs_write_inode(struct super_block *, struct pnlfs_inode *, ino_t,int);
struct pnlfs_inode
*pnlfs_read_inode(struct super_block *sb, ino_t ino);

/* -------------------------------------------------------------------------- */
/* ---------- BFREE --------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int pnlfs_write_block_state(struct super_block *sb, sector_t bno, char val)
{
	struct pnlfs_sb_info	*psbi;
	char			oldval;

	psbi = sb->s_fs_info;
	if (bno < 0 || bno >= psbi->nr_blocks)
		return -EINVAL;
	oldval = pnlfs_read_block_state(sb, bno);
	if (val)
		bitmap_set(psbi->bfree_bitmap, bno, 1);
	else
		bitmap_clear(psbi->bfree_bitmap, bno, 1);
	if (val && !oldval)
		psbi->nr_free_blocks++;
	else if (!val && oldval)
		psbi->nr_free_blocks--;
	return 0;
}

/*
 * Returns the bit corresponding to block 'bno' in the bfree bitmap
 */
int pnlfs_read_block_state(struct super_block *sb, sector_t bno)
{
	struct pnlfs_sb_info	*psbi;
	unsigned int		*rows;
	int			row;
	int			bitno;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (bno < 0 || bno >= psbi->nr_blocks)
		return -EINVAL;

	row = bno / 32;
	bitno = bno % 32;

	/* Reading the block state */
	rows = (unsigned int *)psbi->bfree_bitmap;
	return (rows[row] & (1 << bitno)) != 0;
}

long pnlfs_get_first_free_bno(struct super_block *sb)
{
	struct pnlfs_sb_info	*psbi;
	unsigned long		pos;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	pos = find_first_bit(psbi->bfree_bitmap, psbi->nr_blocks);
	if (pos == psbi->nr_blocks)
		return -1;
	return pos;
}

/* -------------------------------------------------------------------------- */
/* ---------- IFREE --------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int pnlfs_write_inode_state(struct super_block *sb, ino_t ino, char val)
{
	struct pnlfs_sb_info	*psbi;
	char			oldval;

	psbi = sb->s_fs_info;
	if (ino < 0 || ino >= psbi->nr_inodes)
		return -EINVAL;
	oldval = pnlfs_read_block_state(sb, ino);
	if (val)
		bitmap_set(psbi->ifree_bitmap, ino, 1);
	else
		bitmap_clear(psbi->ifree_bitmap, ino, 1);
	if (val && !oldval)
		psbi->nr_free_blocks++;
	else if (!val && oldval)
		psbi->nr_free_blocks--;
	return 0;
}

int pnlfs_read_inode_state(struct super_block *sb, ino_t ino)
{
	struct pnlfs_sb_info	*psbi;
	int			row;
	unsigned int		*rows;
	int			bitno;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (ino < 0 || ino >= psbi->nr_inodes)
		return -EINVAL;

	row = ino / 32;
	bitno = ino % 32;

	/* Reading the inode state */
	rows = (unsigned int *)psbi->ifree_bitmap;
	return (rows[row] & (1 << bitno)) != 0;
}

long pnlfs_get_first_free_ino(struct super_block *sb)
{
	struct pnlfs_sb_info	*psbi;
	unsigned long		pos;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	pos = find_first_bit(psbi->ifree_bitmap, psbi->nr_inodes);
	if (pos == psbi->nr_inodes)
		return -1;
	return pos;
}


/* -------------------------------------------------------------------------- */
/* ---------- ISTORE -------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/*
 * This function writes a pnlfs inode on a pnlfs image
 */
int pnlfs_write_inode(struct super_block *sb,				 // [DONE]
	struct pnlfs_inode *pnli ,ino_t ino, int sync)
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
	if (sync)
		sync_dirty_buffer(bh);
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


