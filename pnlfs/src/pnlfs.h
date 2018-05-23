#ifndef _PNLFS_H
#define _PNLFS_H

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
#include <linux/writeback.h>
//#include <linux/seq_file.h>
//#include <linux/mount.h>
//#include <linux/log2.h>
//#include <linux/quotaops.h>
//#include <asm/uaccess.h>

#define PNLFS_MAGIC           0x434F5746

#define PNLFS_SB_BLOCK_NR              0

#define PNLFS_BLOCK_SIZE       (1 << 12)  /* 4 KiB */
#define PNLFS_MAX_FILESIZE     (1 << 22)  /* 4 MiB */
#define PNLFS_FILENAME_LEN            28
#define PNLFS_MAX_DIR_ENTRIES        128

#define INO_ERR			((unsigned long)-1)
#define INO_UNFOUND		((unsigned long)-2)


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

typedef unsigned long	ulong;

/* ------------ Function prototypes ------------------------------------------*/

/* super.c */
struct inode *pnlfs_alloc_inode(struct super_block *sb);

/* iops.c */
int          pnlfs_iset(struct super_block *sb, struct inode *inode, int sync);
struct inode *pnlfs_iget(struct super_block *sb, unsigned long ino);

/* fops.c */

/* block.c */

/* inode.c */

long	pnlfs_findex_blk(struct inode *inode, int row);
int	pnlfs_free_blk(struct inode *inode, int nb);
int	pnlfs_alloc_blk(struct inode *inode, int nb);

int	pnlfs_dir_rm(struct inode *dir, ino_t ino);
int	pnlfs_dir_add(struct inode *dir, const char *name, ino_t ino);
int	pnlfs_dir_set_name(struct inode *dir, ino_t ino, char *src);
int	pnlfs_dir_get_name(struct inode *dir, ino_t ino, char *dest);
ino_t	pnlfs_dir_get_ino(struct inode *dir, const char *name);

int	pnlfs_write_block_state(struct super_block *sb, sector_t bno, char val);
int	pnlfs_read_block_state(struct super_block *sb, sector_t bno);
long	pnlfs_get_first_free_bno(struct super_block *sb);

int	pnlfs_write_inode_state(struct super_block *sb, ino_t ino, char val);
int	pnlfs_read_inode_state(struct super_block *sb, ino_t ino);
long	pnlfs_get_first_free_ino(struct super_block *sb);

int	pnlfs_write_inode(struct super_block *, struct pnlfs_inode *, ino_t,int);
struct pnlfs_inode *pnlfs_read_inode(struct super_block *sb, ino_t ino);

/* ------------ Inode and file operations ------------------------------------*/

extern struct inode_operations pnlfs_file_inode_operations;
extern struct file_operations pnlfs_file_operations;

#endif	/* _PNLFS_H */
