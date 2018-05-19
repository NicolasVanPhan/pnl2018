
#include <linux/module.h>
//#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
//#include <linux/blkdev.h>
//#include <linux/parser.h>
//#include <linux/random.h>
#include <linux/buffer_head.h>
//#include <linux/exportfs.h>
#include <linux/vfs.h>
//#include <linux/seq_file.h>
//#include <linux/mount.h>
//#include <linux/log2.h>
//#include <linux/quotaops.h>
//#include <asm/uaccess.h>

#include "pnlfs.h"

/*
 * This function reads a pnlfs inode on a pnlfs image
 */
static struct pnlfs_inode *pnlfs_get_inode(struct super_block *sb, ino_t ino)
{
	struct pnlfs_sb_info	*psbi;
	sector_t		blkno;
	struct buffer_head	*bh;
	struct pnlfs_inode	*pnli;
	int			row; // inode row in disk block

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	if (ino < 0 || ino >= psbi->nr_inodes) {
		pr_err("pnlfs_get_inode() : Inode out of range\n");
		return ERR_PTR(-EINVAL);
	}
	blkno = 1 + ino / PNLFS_INODES_PER_BLOCK;
	bh = sb_bread(sb,  blkno);
	if (bh == NULL) {
		pr_err("pnlfs_get_inode() : Block %ld read failed\n", blkno);
		return ERR_PTR(-EIO);
	}
	row = ino % PNLFS_INODES_PER_BLOCK;
	pnli = kmalloc(sizeof(struct pnlfs_inode), GFP_KERNEL);
	if (pnli == NULL) {
		pr_err("pnlfs_get_inode() : Kmalloc failed\n");
		return ERR_PTR(-ENOMEM);
	}
	*pnli = ((struct pnlfs_inode *)bh->b_data)[row];
	brelse(bh);
	return pnli;
}

/*
 * This function gets the inode with number 'ino'
 * If the inode wasn't cached already, it initializes it
 */
struct inode *pnlfs_iget(struct super_block *sb, unsigned long ino)
{
	struct inode		*vfsi;
	struct pnlfs_inode	*pnli;

	vfsi = iget_locked(sb, ino);
	if (vfsi == NULL)
		return (struct inode*)ERR_PTR(-ENOMEM);
	/* If the inode was already cached, return it directly */
	if ((vfsi->i_state & I_NEW) == 0)
		return vfsi;
	/* Otherwise initialize it */
	pnli = pnlfs_get_inode(sb, ino);
	if (IS_ERR(pnli)){
		iget_failed(vfsi);
		return (struct inode *)pnli;
	}
	vfsi->i_mode = le32_to_cpu(pnli->mode);
	vfsi->i_op = &pnlfs_file_inode_operations;
	vfsi->i_fop = &pnlfs_file_operations;
	vfsi->i_sb = sb;
	vfsi->i_ino = ino;
	vfsi->i_size = le32_to_cpu(pnli->filesize);
	vfsi->i_blocks = (blkcnt_t)le32_to_cpu(pnli->nr_used_blocks);
	vfsi->i_atime = CURRENT_TIME;
	vfsi->i_mtime = CURRENT_TIME;
	vfsi->i_ctime = CURRENT_TIME;

	kfree(pnli);
	unlock_new_inode(vfsi);
	return vfsi;
}

struct inode_operations pnlfs_file_inode_operations = {};
struct file_operations pnlfs_file_operations = {};

