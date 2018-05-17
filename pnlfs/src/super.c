
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


MODULE_AUTHOR("Nicolas Phan");
MODULE_DESCRIPTION("A filesystem");
MODULE_LICENSE("GPL");

/* -------------------------------------------------------------------------- */
/* --------- Registering the file system ------------------------------------ */
/* -------------------------------------------------------------------------- */

static struct file_system_type pnlfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "pnlfs",
	.mount		= NULL,
	.kill_sb	= NULL,
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
