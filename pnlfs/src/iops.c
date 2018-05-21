
#include "pnlfs.h"

/*
struct inode *pnlfs_iget(struct super_block *sb, unsigned long ino)
*/

static struct dentry
*pnlfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags);

static int
pnlfs_create(struct inode *dir, struct dentry *de, umode_t mode, bool excl);

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
	ino = pnlfs_get_ino_from_name(dir, dentry->d_name.name);
	if (ino < 0)
		return ERR_PTR(ino);

	/* Get the inode of the target file */
	inode = pnlfs_iget(dir->i_sb, ino);
	if (IS_ERR(inode))
		return (struct dentry *)inode;

	/* Fill the dentry from the retrieved inode */
	d_add(dentry, inode);
	return dentry;
}

/*
 * This function creates a new inode and attaches it to the the negative dentry
 * given as a parameter.
 */
static int pnlfs_create(struct inode *dir, struct dentry *de,
	umode_t mode, bool excl)
{
	struct inode	*inode;
	ino_t		ino;

	/* Get a new inode */
	ino = pnlfs_get_first_free_ino(dir->i_sb);
	if (ino < 0)
		return -1;
	inode = pnlfs_alloc_inode(dir->i_sb);
	if (inode == NULL)
		return -1;
	inode->i_mode = mode;
	inode->i_op = &pnlfs_file_inode_operations;
	inode->i_fop = &pnlfs_file_operations;
	inode->i_sb = dir->i_sb;
	inode->i_ino = ino;
	inode->i_size = 0;
	inode->i_blocks = 0;
	inode->i_atime = CURRENT_TIME;
	inode->i_mtime = CURRENT_TIME;
	inode->i_ctime = CURRENT_TIME;

	mark_inode_dirty(inode);
	d_instantiate(de, inode);
	return 0;
}

struct inode_operations pnlfs_file_inode_operations = {
	.lookup = pnlfs_lookup,
	.create = pnlfs_create,
};

