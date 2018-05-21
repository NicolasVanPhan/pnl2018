
/*
 * pnlfs/src/super.c
 *
 * PNL 2017/2018
 * Nicolas Phan <nicolas.van.phan@gmail.com>
 *
 */

#include "pnlfs.h"

MODULE_AUTHOR("Nicolas Phan");
MODULE_DESCRIPTION("A filesystem");
MODULE_LICENSE("GPL");

/*
struct inode *pnlfs_alloc_inode(struct super_block *sb);
*/

static void pnlfs_put_super(struct super_block *sb);
static void pnlfs_destroy_inode(struct inode *vfsi);
static int pnlfs_fill_super(struct super_block *sb, void *data, int silent);
static struct dentry *pnlfs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data);

/* -------------------------------------------------------------------------- */
/* --------- Superblock Operations ------------------------------------------ */
/* -------------------------------------------------------------------------- */

/*
 * This function undoes all what has been done by fill_super()
 */
static void pnlfs_put_super(struct super_block *sb)
{
	struct pnlfs_sb_info *psbi;

	psbi = (struct pnlfs_sb_info *)sb->s_fs_info;
	kfree(psbi->ifree_bitmap);
	kfree(psbi->bfree_bitmap);
	kfree(psbi);
	sb->s_fs_info = NULL;
}

/*
 * This functions allocates a new pnlfs inode
 */
struct inode *pnlfs_alloc_inode(struct super_block *sb)
{
	struct pnlfs_inode_info *pnli;

	pnli = kmalloc(sizeof(struct pnlfs_inode_info), GFP_KERNEL);
	if (pnli == NULL) {
		pr_err("pnlfs: pnlfs_alloc_inode() : kmalloc failed\n");
		return NULL;
	}
	inode_init_once(&pnli->vfs_inode);
	pnli->index_block = 0;
	pnli->nr_entries = 0;
	return &pnli->vfs_inode;
}

/*
 * This function destroys a pnlfs inode
 */
static void pnlfs_destroy_inode(struct inode *vfsi)
{
	struct pnlfs_inode_info *pnli;

	pnli = container_of(vfsi, struct pnlfs_inode_info, vfs_inode);
	kfree(pnli);
}

/*
 * This structure contains pointers to all the functions performing
 * a superblock operation
 */
static const struct super_operations pnlfs_sops = {
	.put_super = pnlfs_put_super,
	.alloc_inode = pnlfs_alloc_inode,
	.destroy_inode = pnlfs_destroy_inode,
};

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
	ulong			*ifree_bitmap;
	ulong			*bfree_bitmap;
	ulong			*pcur_row;
	int			nb_rows_per_block;
	int			i;
	struct inode		*root;

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
	
	/* Extracting superblock content (the i bitmaps) */
	ifree_bitmap = kmalloc(PNLFS_BLOCK_SIZE * psbi->nr_ifree_blocks,
		GFP_KERNEL);
	blkfirst = 1 + psbi->nr_istore_blocks; // first ifree block number
	blklast = blkfirst + psbi->nr_ifree_blocks; // last ifree block number
	nb_rows_per_block = PNLFS_BLOCK_SIZE / sizeof(ulong);
	pcur_row = ifree_bitmap;
	for (blkno = blkfirst; blkno < blklast; blkno++) { // foreach bitmap blk
		bh = sb_bread(sb, blkno); // get the bitmap block
		for (i = 0; i < nb_rows_per_block; i++) // save the rows
			*pcur_row++ = le64_to_cpu(((ulong *)bh->b_data)[i]);
	}
	psbi->ifree_bitmap = ifree_bitmap;

	/* Extracting superblock content (the b bitmaps) */
	bfree_bitmap = kmalloc(PNLFS_BLOCK_SIZE * psbi->nr_bfree_blocks,
		GFP_KERNEL);
	blkfirst = 1 + psbi->nr_istore_blocks + psbi->nr_ifree_blocks;
	blklast = blkfirst + psbi->nr_bfree_blocks; // last ifree block number
	nb_rows_per_block = PNLFS_BLOCK_SIZE / sizeof(ulong);
	pcur_row = bfree_bitmap;
	for (blkno = blkfirst; blkno < blklast; blkno++) { // foreach bitmap blk
		bh = sb_bread(sb, blkno); // get the bitmap block
		for (i = 0; i < nb_rows_per_block; i++) // save the rows
			*pcur_row++ = le64_to_cpu(((ulong *)bh->b_data)[i]);
	}
	psbi->bfree_bitmap = bfree_bitmap;

	/* Attaching our superblock operations to the VFS superblock */ 
	sb->s_op = &pnlfs_sops;

	/* Attaching the extracted content into the generic structure sb */
	sb->s_fs_info = (void *)psbi;

	/* Getting the root inode */
	root = pnlfs_iget(sb, 0);
	if (IS_ERR(root))
		return PTR_ERR(root);
	inode_init_owner(root, root, root->i_mode); // [TMP] parent is root
	sb->s_root = d_make_root(root);
	if (!sb->s_root) {
		pr_err("pnlfs_fill_super(): Error getting root inode\n");
		return -1;
	}

	brelse(bh);
	return 0;
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
