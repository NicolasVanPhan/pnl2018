
#include "pnlfs.h"

long	pnlfs_findex_blk(struct inode *inode, int row);
int	pnlfs_free_blk(struct inode *inode, int nb);
int	pnlfs_alloc_blk(struct inode *inode, int nb);

/* -------------------------------------------------------------------------- */
/* ---------- DATA : FILES -------------------------------------------------- */
/* -------------------------------------------------------------------------- */

long	pnlfs_findex_blk(struct inode *inode, int row)
{
	struct pnlfs_inode_info		*pnli;
	struct super_block		*sb;
	struct pnlfs_sb_info		*psbi;
	struct buffer_head		*bh;
	struct pnlfs_file_index_block	*dblk;
	long				bno;

	pnli = container_of(inode, struct pnlfs_inode_info, vfs_inode);
	sb = inode->i_sb;
	psbi = sb->s_fs_info;

	/* Treat errors */
	if (row < 0 || row >= pnli->nr_entries) {
		pr_err("pnlfs_findex_blk: Invalid row value\n");
		return -EINVAL;
	}

	/* Get file index block */
	bh = sb_bread(sb, pnli->index_block);
	if (bh == NULL) {
		pr_err("pnlfs_findex_blk : File index block unreadable\n");
		return -1;
	}

	/* Get block number at row 'row' */
	dblk = (struct pnlfs_file_index_block*)bh->b_data;
	bno = le32_to_cpu(dblk->blocks[row]);

	return bno;
}

int	pnlfs_free_blk(struct inode *inode, int nb)
{
	struct pnlfs_inode_info		*pnli;
	struct super_block		*sb;
	struct pnlfs_sb_info		*psbi;
	struct buffer_head		*bh;
	struct pnlfs_file_index_block	*dblk;
	long				bno;
	int				i;

	pnli = container_of(inode, struct pnlfs_inode_info, vfs_inode);
	sb = inode->i_sb;
	psbi = sb->s_fs_info;

	/* Treat errors */
	if (nb > pnli->nr_entries) {
		pr_err("pnlfs_free_blk : Free more than filesize\n");
		return -1;
	}

	/* Get file index block */
	bh = sb_bread(sb, pnli->index_block);
	if (bh == NULL) {
		pr_err("pnlfs_free_blk : File index block unreadable\n");
		return -1;
	}
	dblk = (struct pnlfs_file_index_block*)bh->b_data;

	/* Free 'nb' last blocks */
	for (i = 0; i < nb; i++) {
		/* Detach block from our file */
		bno = le32_to_cpu(dblk->blocks[pnli->nr_entries -1 - i]);
		dblk->blocks[pnli->nr_entries -1 - i] = 0;
		/* Free it */
		pnlfs_write_block_state(sb, bno, 1);
	}
	pnli->nr_entries -= nb;
	mark_inode_dirty(inode);
	return 0;
}

int	pnlfs_alloc_blk(struct inode *inode, int nb)
{
	struct pnlfs_inode_info		*pnli;
	struct super_block		*sb;
	struct pnlfs_sb_info		*psbi;
	struct buffer_head		*bh;
	struct pnlfs_file_index_block	*dblk;
	long				bno;
	int				i;

	pnli = container_of(inode, struct pnlfs_inode_info, vfs_inode);
	sb = inode->i_sb;
	psbi = sb->s_fs_info;

	/* Treat errors */
	if (pnli->nr_entries + nb > PNLFS_BLOCK_SIZE >> 2) {
		pr_err("pnlfs_alloc_blk : File will be too large\n");
		return -1;
	}

	if (nb > psbi->nr_free_blocks) {
		pr_err("pnlfs_alloc_blk : Disk is full\n");
		return -1;
	}

	/* Get file index block */
	bh = sb_bread(sb, pnli->index_block);
	if (bh == NULL) {
		pr_err("pnlfs_alloc_blk : File index block unreadable\n");
		return -1;
	}
	dblk = (struct pnlfs_file_index_block*)bh->b_data;

	/* Alloc 'nb' new blocks */
	for (i = 0; i < nb; i++) {
		/* Reserve a new block */
		bno = pnlfs_get_first_free_bno(sb);
		if (bno <= 0)
			return -1;
		pnlfs_write_block_state(sb, bno, 0);
		/* Attach it to our file */
		dblk->blocks[pnli->nr_entries + i] = cpu_to_le32(bno);
	}
	pnli->nr_entries += nb;
	mark_inode_dirty(inode);
	return 0;
}

