
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
//#include <linux/slab.h>
#include <linux/init.h>
//#include <linux/blkdev.h>
//#include <linux/parser.h>
//#include <linux/random.h>
//#include <linux/buffer_head.h>
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
 * This function is needed by pnlfs_mount(), it setups the super block
 */
static int pnlfs_fill_super(struct super_block *sb, void *data, int silent)
{
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
