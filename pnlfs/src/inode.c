
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

static struct pnlfs_inode *pnlfs_get_inode(struct super_block *sb, ino_t ino);
struct inode *pnlfs_iget(struct super_block *sb, unsigned long ino);
static struct dentry *pnlfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags);

ino_t get_ino_from_name(struct inode *dir, const char *name)
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
		return -EINVAL;
	}

	if (dir == NULL)
		return -EFAULT;

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the ino matching our filename */
	ret = -1;
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
	struct pnlfs_inode_info	*pnlii;

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
	pnlii = container_of(vfsi, struct pnlfs_inode_info, vfs_inode);
	pnlii->index_block = le32_to_cpu(pnli->index_block);
	pnlii->nr_entries = le32_to_cpu(pnli->nr_entries);

	kfree(pnli);
	unlock_new_inode(vfsi);
	return vfsi;
}


/* -------------------------------------------------------------------------- */
/* --------- INODE OPERATIONS ----------------------------------------------- */
/* -------------------------------------------------------------------------- */

/*
 * This function returns the dentry refering to the file named after
 * the name found in dentry->d_name, in the parent directory refered
 * by dir
 *
 * @dir:    The inode of the parent directory in which this function must
 *          find the file named dentry->d_name
 * @dentry: The negative (="empty", "yet-to-be-filled") dentry of the file
 *          whose inode must be found by this function
 */
static struct dentry *pnlfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags)
{
	ino_t		ino;
	struct inode	*inode;

	/* Find the inode number of the file to be found (aka target file) */
	ino = get_ino_from_name(dir, dentry->d_name.name);

	/* Get the inode of the target file */
	inode = pnlfs_iget(dir->i_sb, ino);

	/* Fill the dentry from the retrieved inode */
	d_add(dentry, inode);
	return dentry;
}

struct inode_operations pnlfs_file_inode_operations = {
	.lookup = pnlfs_lookup,
};

