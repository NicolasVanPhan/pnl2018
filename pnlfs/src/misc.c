
#include "pnlfs.h"

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

/* FOR THE MOMENT THIS FUNCTION IS JUST AN INTERFACE FOR TESTING INODE.C */
static int pnlfs_rename(struct inode* idir, struct dentry *ddir,
	struct inode *ifile, struct dentry *dfile, unsigned int n)
{
	long			rsp;
	char			str[50];
	struct buffer_head	*bh;
	void			*bdata;
	unsigned int		*rows;
	struct pnlfs_sb_info	*psb = idir->i_sb->s_fs_info;
	sector_t		blkis = 1;
	sector_t		blkif = blkis + psb->nr_istore_blocks;
	sector_t		blkbf = blkif + psb->nr_ifree_blocks;
	sector_t		blkdt = blkbf + psb->nr_bfree_blocks;
	int			i;

	// GDB variables
	int			gdbloop = 1;
	int			cmd;
	long			no;
	long			state;
	struct pnlfs_inode 	pi2write;
	struct pnlfs_inode 	*pi2read;
	char			name[PNLFS_FILENAME_LEN];


	while (gdbloop) {
		// BREAKPOINT (MODIFY cmd TO EXECUTE FN OR gdbloop TO EXIT)

		switch (cmd) {
			case 0: // dump the disk
				bh = sb_bread(idir->i_sb, no);
				bdata = bh->b_data;
				// Convert endianness
				rows = kmalloc(PNLFS_BLOCK_SIZE, GFP_KERNEL);
				for (i = 0; i < 256; i++)
					rows[i] = le32_to_cpu(((unsigned int *)bdata)[i]);
				brelse(bh);
				kfree(rows);
				break;
			case 1:	// read_inode_state
				rsp = pnlfs_read_inode_state(idir->i_sb, no);
				pr_info("rsp: %ld\n", rsp);
				break;
			case 2: // write_inode_state
				pnlfs_write_inode_state(idir->i_sb, no, state);
				break;
			case 3: // read inode
				pi2read = pnlfs_read_inode(idir->i_sb, no);
				if (!IS_ERR(pi2read))
					kfree(pi2read);
				break;
			case 4: // write inode
				rsp = pnlfs_write_inode(idir->i_sb, &pi2write, no, 0);
				pr_info("rsp: %ld\n", rsp);
				break;
			case 5: // get_first_free_ino
				rsp = pnlfs_get_first_free_ino(idir->i_sb);
				pr_info("rsp: %ld\n", rsp);
				break;
			case 6: // read_block_state
				rsp = pnlfs_read_block_state(idir->i_sb, no);
				pr_info("rsp: %ld\n", rsp);
				break;
			case 7: // write_block_state
				pnlfs_write_block_state(idir->i_sb, no, state);
				break;
			case 8: // get_first_free_bno
				rsp = pnlfs_get_first_free_bno(idir->i_sb);
				pr_info("rsp: %ld\n", rsp);
				break;
			case 9: // dir_get_ino					 ok 
				rsp = pnlfs_dir_get_ino(idir, name);
				pr_info("ino : %ld\n", rsp);
				break;
			case 10: // dir_add
				rsp = pnlfs_dir_add(idir, name, no);
				pr_info("rsp : %ld\n", rsp);
				break;
			case 11: // dir_rm
				rsp = pnlfs_dir_rm(idir, no);
				pr_info("rsp : %ld\n", rsp);
				break;
			case 12: // dir_set_name
				rsp = pnlfs_dir_set_name(idir, no, name);
				pr_info("rsp : %ld\n", rsp);
				break;
			case 13: // dir_get_name				 ok
				rsp = pnlfs_dir_get_name(idir, no, name);
				pr_info("rsp : %ld\n", rsp);
				break;
			case 14: // alloc_blk
				rsp = pnlfs_alloc_blk(ifile, no);
				pr_info("rsp : %ld\n", rsp);
				break;
			case 15: // free_blk
				rsp = pnlfs_free_blk(ifile, no);
				pr_info("rsp : %ld\n", rsp);
				break;
			case 16: // findex_blk
				rsp = pnlfs_findex_blk(ifile, no);
				pr_info("rsp : %ld\n", rsp);
				break;
		}
	}

	return -EINVAL;
}

