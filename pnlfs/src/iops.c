
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
	pnli = pnlfs_read_inode(sb, ino);
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
	ino = pnlfs_dir_get_ino(dir, dentry->d_name.name);
	if (ino == INO_ERR || ino == INO_UNFOUND) {
		d_add(dentry, NULL);
		return dentry;
	}

	/* Get the inode of the target file */
	inode = pnlfs_iget(dir->i_sb, ino);
	if (IS_ERR(inode)) {
		d_add(dentry, NULL);
		return dentry;
	}

	/* Fill the dentry from the retrieved inode */
	d_add(dentry, inode);
	return dentry;
}

/*
 * This function creates a new inode and attaches it to the the negative dentry
 * given as a parameter.
 */
/*
static int pnlfs_create(struct inode *dir, struct dentry *de,
	umode_t mode, bool excl)
{
	struct inode		*inode;
	struct pnlfs_inode_info *pnlii;
	struct pnlfs_dir_block	*pnldb;
	struct pnlfs_file	*pnlentries;
	struct pnlfs_file	*entry;
	ino_t			ino;

	// Get a new inode
	ino = pnlfs_get_first_free_ino(dir->i_sb)
	if (ino < 0)
		return -1;
	inode = pnlfs_alloc_inode(dir->i_sb);
	if (inode == NULL)
		return -1;

	// Fill the new inode
	inode->i_mode = mode;
	inode->i_op = &pnlfs_file_inode_operations;
	inode->i_fop = &pnlfs_file_operations;
	inode->i_sb = dir->i_sb;
	inode->i_ino = ino;
	inode->i_atime = CURRENT_TIME;
	inode->i_mtime = CURRENT_TIME;
	inode->i_ctime = CURRENT_TIME;
	inode->i_size = 0;
	inode->i_blocks = 0;
	mark_inode_dirty(inode);

	// Get the disk block of the directory
	inode = pnlfs_iget(dir->sb, dir->i_ino);
	if (IS_ERR(inode))
		return -1;
	pnlii = container_of(inode, struct pnlfs_inode_info, vfs_inode);
	pnldb = pnlfs_read_dir_block(dir->i_sb, pnlii->index_block);
	if (IS_ERR(pnldb))
		return -1;

	// Add the new file to the directory in the disk
	pnlentries = (struct pnlfs_files *)pnldb;
	if (pnlii->nr_entries >= PNLFS_MAX_DIR_ENTRIES) {
		pr_err("Too many entries in this directory\n");
		return -1;
	}
	entry = &pnlfiles[pnlii->nr_entries];
	entry->inode = cpu_to_le32(ino);
	strncpy(entry->filename, de->d_name.name, PNLFS_FILENAME_LEN)
	pnlii->nr_entries++;

	//Write back all changes
	pnlfs_write_dir_block(dir->i_sb, pnlii->index_block, pnldb);
	mark_buffer_dirty(inode);
	pnlfs_write_inode_state(dir->i_sb, ino, 0);
	d_instantiate(de, inode);
	return 0;
}

*/

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
				rsp = pnlfs_write_inode(idir->i_sb, &pi2write, no);
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
		}
	}

	return -EINVAL;
}

struct inode_operations pnlfs_file_inode_operations = {
	.lookup = pnlfs_lookup,
//	.create = pnlfs_create,
	.rename = pnlfs_rename,
};

