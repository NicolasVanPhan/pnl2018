
#include "pnlfs.h"

static int pnlfs_iterate_shared(struct file *file, struct dir_context *ctx);		// dir.c

/* -------------------------------------------------------------------------- */
/* --------- FILE OPERATIONS ------------------------------------------------ */
/* -------------------------------------------------------------------------- */

/*
 * This function fills 'ctx' with all the files present in the dir 'file'
 */
static int pnlfs_iterate_shared(struct file *fdir, struct dir_context *ctx)
{
	struct inode		*idir;
	struct pnlfs_inode_info	*pnli;
	int			nr_entries;
	sector_t		blkno;
	struct buffer_head	*bh;
	struct pnlfs_file	*rows;
	char			*name;
	int			namelen;
	ino_t			ino;
	unsigned char		d_type;
	int			i;

	if (ctx->pos)
		return 0;

	/* Get the block where our directory's filelist is stored */
	idir = file_inode(fdir);
	pnli = container_of(idir, struct pnlfs_inode_info,  vfs_inode);
	if (pnli == NULL)
		return 0;
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(idir->i_sb, blkno);

	/* Add the . and .. directories */
	if (!dir_emit_dots(fdir, ctx))
		return 0;

	/* Iterate over our filelist */
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		name = rows[i].filename;
		namelen = strnlen(name, PNLFS_FILENAME_LEN);
		if (!namelen)
			continue;
		ino = rows[i].inode;
		d_type = DT_UNKNOWN;
		if(dir_emit(ctx, name, namelen, ino, d_type))
			ctx->pos += sizeof(struct pnlfs_file);
	}

	brelse(bh);
	return 0;
}

static ssize_t
pnlfs_write(struct file* file, const char __user *src, size_t len, loff_t *off)
{
	struct inode *inode;
	struct pnlfs_inode_info *pnlii;
	struct buffer_head *bh;
	int	nb_needed_blocks;
	int	nb_char_per_blk;
	int	nb_blk_to_alloc;
	int	blkfirst;
	int	blklast;
	int	bno;
	const char	*from;
	char	*to;
	int	offset;
	int	length;
	char	eof;

	if (*off < 0)
		return 0;

	nb_char_per_blk = PNLFS_BLOCK_SIZE;
	inode = file->f_inode;
	pnlii = container_of(inode, struct pnlfs_inode_info, vfs_inode);

	/* Alloc additionnal blocks if needed, to store what's written */
	nb_needed_blocks = (*off + len + nb_char_per_blk - 1) / nb_char_per_blk;
	nb_blk_to_alloc = nb_needed_blocks - pnlii->nr_entries;;
	if (pnlfs_alloc_blk(inode, nb_blk_to_alloc))
		return 0;

	blkfirst = pnlfs_findex_blk(inode, *off / PNLFS_BLOCK_SIZE);
	blklast = pnlfs_findex_blk(inode, *off + len / PNLFS_BLOCK_SIZE);

	/* Go to block according to current position */
	bno = pnlfs_findex_blk(inode, *off / PNLFS_BLOCK_SIZE);

	if (bno < 0) {
		pr_err("pnlfs_write() : Cannot find file block");
		return 0;
	}

	/* Determine how many char must be written to this block */
	length = nb_char_per_blk - *off % nb_char_per_blk;
	length = len < length ? len : length;

	/* Determine the block offset from which we must write */
	offset = *off % nb_char_per_blk;

	/* Open the block for writing */
	bh = sb_bread(inode->i_sb, bno);
	if (bh == NULL) {
		pr_err("pnlfs_write() : Cannot read block %d\n", bno);
		return 0;
	}

	/* Write ! */
	from = src;
	to = &((char*)bh->b_data)[offset];
	if (copy_from_user(to, from, length - 1)) {
		pr_err("pnlfs_write() : copy from user failed\n");
		return -EIO;
	}
	/* Not forgetting to add a \0 */
	/*
	eof = '\0';
	from = &eof;
	strncpy(to + length - 1, from, 1);
	*/
	to[length - 1] = '\0';

	pr_info("bno: %d, len: %d, off: %d\n", bno, length, offset);

	/* Update cursor position */
	*off += length;

	mark_buffer_dirty(bh);
	brelse(bh);
	return length;
}



static ssize_t
pnlfs_write_backup(struct file* file, const char __user *src, size_t len, loff_t *off)
{
	struct inode *inode;
	struct pnlfs_inode_info *pnlii;
	struct buffer_head *bh;
	int	nb_needed_blocks;
	int	nb_char_per_blk;
	int	nb_blk_to_alloc;
	int	blkfirst;
	int	blklast;
	int	bno;
	const char	*from;
	char	*to;
	int	offset;
	int	length;

	nb_char_per_blk = PNLFS_BLOCK_SIZE;
	inode = file->f_inode;
	pnlii = container_of(inode, struct pnlfs_inode_info, vfs_inode);

	/* Alloc additionnal blocks to store what's written, if needed */
	nb_needed_blocks = (*off + len + nb_char_per_blk - 1) / nb_char_per_blk;
	nb_blk_to_alloc = nb_needed_blocks - pnlii->nr_entries;
	if (pnlfs_alloc_blk(inode, nb_blk_to_alloc))
		return -1;

	blkfirst = pnlfs_findex_blk(inode, *off / PNLFS_BLOCK_SIZE);
	blklast = pnlfs_findex_blk(inode, *off + len / PNLFS_BLOCK_SIZE);

	/* For each block to write to */
	do {

		/* Go to block according to current position */
		bno = pnlfs_findex_blk(inode, *off / PNLFS_BLOCK_SIZE);

		/* Determine how many char must be written to this block */
		if (bno == blkfirst)
			length = nb_char_per_blk - *off % nb_char_per_blk;
		if (bno == blklast)
			length = (*off + len) % nb_char_per_blk;
		else
			length = nb_char_per_blk;
		/* Determine the block offset from which we must write */
		if (bno == blkfirst)
			offset = *off % nb_char_per_blk;
		else
			offset = 0;
		/* Open the block for writing */
		bh = sb_bread(inode->i_sb, bno);
		if (bh == NULL) {
			pr_err("pnlfs_write() : Cannot read block %d\n", bno);
			return -1;
		}

		/* Write ! */
		from = src;
		to = &((char*)bh->b_data)[offset];
		if (copy_from_user(to, from, length)) {
			pr_err("pnlfs_write() : copy from user failed\n");
			return -EIO;
		}

		pr_info("bno: %d, len: %d, off: %d\n", bno, length, offset);

		/* Update cursor position */
		*off += length;

		mark_buffer_dirty(bh);
		brelse(bh);
	} while (bno != blklast);
	return len;
}

static ssize_t
pnlfs_read(struct file* file, char __user *dest, size_t len, loff_t *off)
{
	struct inode		*inode;
	struct buffer_head	*bh;
	int			nb_char_per_blk;
	int			bno;
	const char		*from;
	char			*to;
	int			length;

	inode = file->f_inode;
	nb_char_per_blk = PNLFS_BLOCK_SIZE;

	if (*off < 0)
		return 0;

	/* Get the right data block according to current position */
	bno = pnlfs_findex_blk(inode, *off / PNLFS_BLOCK_SIZE);

	/* If cursor is already beyond file end, quit */
	if (bno < 0)
		return 0;

	length = nb_char_per_blk - *off % nb_char_per_blk;
	length = len < length ? len : length;

	/* Open the block for reading */
	bh = sb_bread(inode->i_sb, bno);
	if (bh == NULL) {
		pr_err("pnlfs_read() : Cannot read block %d\n", bno);
		return -1;
	}

	/* Read ! */
	to = dest;
	from = &((char*)bh->b_data)[*off];
	if (copy_to_user(to, from, length)) {
		pr_err("pnlfs_read() : copy to user failed\n");
		return -EIO;
	}

	pr_info("bno: %d, len: %d, off: %lld\n", bno, length, *off);

	/* Update cursor position */
	*off += length;

	brelse(bh);
	return length;
}

struct file_operations pnlfs_file_operations = {
	.iterate_shared = pnlfs_iterate_shared,
	.write		= pnlfs_write,
	.read		= pnlfs_read,
};

