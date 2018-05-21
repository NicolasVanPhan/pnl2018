
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

	/* Add the .. directory */
	if (!dir_emit_dots(fdir, ctx))
		return 0;

	/* Get the block where our directory's filelist is stored */
	idir = file_inode(fdir);
	pnli = container_of(idir, struct pnlfs_inode_info,  vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(idir->i_sb, blkno);

	/* Iterate over our filelist */
	rows = (struct pnlfs_file *)bh->b_data;
	i = ctx->pos - 2;
	if (i < nr_entries) {
		name = rows[i].filename;
		namelen = strnlen(name, PNLFS_FILENAME_LEN);
		ino = rows[i].inode;
		d_type = DT_UNKNOWN;
		if(dir_emit(ctx, name, namelen, ino, d_type))
			ctx->pos++;
	}

	brelse(bh);
	return 0;
}

struct file_operations pnlfs_file_operations = {
	.iterate_shared = pnlfs_iterate_shared,
};

