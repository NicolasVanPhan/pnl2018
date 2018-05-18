
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

//#include "pnlfs.h"







#ifndef _PNLFS_H
#define _PNLFS_H

#define PNLFS_MAGIC           0x434F5746

#define PNLFS_SB_BLOCK_NR              0

#define PNLFS_BLOCK_SIZE       (1 << 12)  /* 4 KiB */
#define PNLFS_MAX_FILESIZE     (1 << 22)  /* 4 MiB */
#define PNLFS_FILENAME_LEN            28
#define PNLFS_MAX_DIR_ENTRIES        128


/*
 * pnlFS partition layout
 *
 * +---------------+
 * |  superblock   |  1 block
 * +---------------+
 * |  inode store  |  sb->nr_istore_blocks blocks
 * +---------------+
 * | ifree bitmap  |  sb->nr_ifree_blocks blocks
 * +---------------+
 * | bfree bitmap  |  sb->nr_bfree_blocks blocks
 * +---------------+
 * |    data       |
 * |      blocks   |  rest of the blocks
 * +---------------+
 *
 */

struct pnlfs_inode {
	__le32 mode;		  /* File mode */
	__le32 index_block;	  /* Block with list of blocks for this file */
	__le32 filesize;	  /* File size in bytes */
	union {
		__le32 nr_used_blocks;  /* Number of blocks used by file */
		__le32 nr_entries;     /* Number of files/dirs in directory */
	};
};

struct pnlfs_inode_info {
	uint32_t index_block;
	uint32_t nr_entries;
	struct inode vfs_inode;
};

#define PNLFS_INODES_PER_BLOCK (PNLFS_BLOCK_SIZE / sizeof(struct pnlfs_inode))

struct pnlfs_superblock {
	__le32 magic;	        /* Magic number */

	__le32 nr_blocks;       /* Total number of blocks (incl sb & inodes) */
	__le32 nr_inodes;       /* Total number of inodes */

	__le32 nr_istore_blocks;/* Number of inode store blocks */
	__le32 nr_ifree_blocks; /* Number of inode free bitmap blocks */
	__le32 nr_bfree_blocks; /* Number of block free bitmap blocks */

	__le32 nr_free_inodes;  /* Number of free inodes */
	__le32 nr_free_blocks;  /* Number of free blocks */

	char padding[4064];     /* Padding to match block size */
};

struct pnlfs_sb_info {
	uint32_t nr_blocks;      /* Total number of blocks (incl sb & inodes) */
	uint32_t nr_inodes;      /* Total number of inodes */

	uint32_t nr_istore_blocks;/* Number of inode store blocks */
	uint32_t nr_ifree_blocks; /* Number of inode free bitmap blocks */
	uint32_t nr_bfree_blocks; /* Number of block free bitmap blocks */

	uint32_t nr_free_inodes;  /* Number of free inodes */
	uint32_t nr_free_blocks;  /* Number of free blocks */

	unsigned long *ifree_bitmap;
	unsigned long *bfree_bitmap;
};

struct pnlfs_file_index_block {
	__le32 blocks[PNLFS_BLOCK_SIZE >> 2];
};

struct pnlfs_dir_block {
	struct pnlfs_file {
		__le32 inode;
		char filename[PNLFS_FILENAME_LEN];
	} files[PNLFS_MAX_DIR_ENTRIES];
};

#endif	/* _PNLFS_H */










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
	sector_t		blkno;
	sector_t		blkfirst;
	sector_t		blklast;
	unsigned long		*ifree_bitmap;
	//unsigned long		*bfree_bitmap;
	unsigned long		*pcur_raw;
	int			nb_raws_per_block;
	int			i;

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
	
	/* Extracting superblock content (the bitmaps) */
	ifree_bitmap = kmalloc(PNLFS_BLOCK_SIZE * psbi->nr_ifree_blocks,
		GFP_KERNEL);
	blkfirst = 1 + psbi->nr_istore_blocks; // first ifree block number
	blklast = blkfirst + psbi->nr_ifree_blocks; // last ifree block number
	nb_raws_per_block = PNLFS_BLOCK_SIZE / sizeof(unsigned long);
	pcur_raw = ifree_bitmap;
	for (blkno = blkfirst; blkno < blklast; blkno++) { // foreach bitmap blk
		bh = sb_bread(sb, blkno); // get the bitmap block
		for (i = 0; i < nb_raws_per_block; i++) // save the raws
			*pcur_raw++ = ((unsigned long *)bh->b_data)[i];
	}
	psbi->ifree_bitmap = ifree_bitmap;

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
