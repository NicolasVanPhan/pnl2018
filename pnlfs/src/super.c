
/*
 * pnlfs/src/super.c
 *
 * PNL 2017/2018
 * Nicolas Phan <nicolas.van.phan@gmail.com>
 *
 */
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

MODULE_AUTHOR("Nicolas Phan");
MODULE_DESCRIPTION("A filesystem");
MODULE_LICENSE("GPL");

/* -------------------------------------------------------------------------- */
/* --------- Mounting the file system --------------------------------------- */
/* -------------------------------------------------------------------------- */

/*
 * This function is needed by pnlfs_mount(), it reads the pnlfs disk superblock
 * and fills the VFS superblock variable consequently
 */
static int pnlfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct buffer_head	*bh;
	struct pnlfs_superblock	*psb;
	struct pnlfs_sb_info	*psbi;

	/* Initialize the super block given as a parameter */
	sb->s_magic = PNLFS_MAGIC;
	sb->s_blocksize = PNLFS_BLOCK_SIZE;
	sb->s_maxbytes = PNLFS_MAX_FILESIZE;

	/* Reading the superblock */
	bh = sb_bread(sb, 0); // Reading the disk.img superblock
	if (bh == NULL) {
		pr_err("pnlfs: The superblock is unreadable\n");
		return -1;
	}

	/* Extracting superblock content */
	psb = (struct pnlfs_superblock *)bh->b_data;
	psbi = kmalloc(sizeof(struct pnlfs_sb_info), GFP_KERNEL);
	psbi->nr_blocks = le32_to_cpu(psb->nr_blocks);
	psbi->nr_inodes = le32_to_cpu(psb->nr_inodes);
	psbi->nr_istore_blocks = le32_to_cpu(psb->nr_istore_blocks);
	psbi->nr_ifree_blocks = le32_to_cpu(psb->nr_ifree_blocks);
	psbi->nr_bfree_blocks = le32_to_cpu(psb->nr_bfree_blocks);
	psbi->nr_free_inodes = le32_to_cpu(psb->nr_free_inodes);
	psbi->nr_free_blocks = le32_to_cpu(psb->nr_free_blocks);

	/* Attaching the extracted content into the generic structure sb */
	sb->s_fs_info = (void *)psbi;

	brelse(bh);
	return -1;
}

/*
 * This function is called by the VFS when a user mounts a pnlfs image
 */
static struct dentry *pnlfs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, pnlfs_fill_super);
}

/* -------------------------------------------------------------------------- */
/* --------- Registering the file system ------------------------------------ */
/* -------------------------------------------------------------------------- */

static struct file_system_type pnlfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "pnlfs",
	.mount		= pnlfs_mount,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV, /* NOTE : Our FS requires a device */
};

static int __init init_pnlfs(void)
{
	int err;

	err = register_filesystem(&pnlfs_fs_type);
	if (err)
		return err;
	return 0;
}

static void __exit exit_pnlfs(void)
{
	unregister_filesystem(&pnlfs_fs_type);
}

module_init(init_pnlfs);
module_exit(exit_pnlfs);
