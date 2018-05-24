
#include "pnlfs.h"

int	pnlfs_dir_rm(struct inode *dir, ino_t ino);
int	pnlfs_dir_add(struct inode *dir, const char *name, ino_t ino);
int	pnlfs_dir_set_name(struct inode *dir, ino_t ino, char *src);
int	pnlfs_dir_get_name(struct inode *dir, ino_t ino, char *dest);
ino_t	pnlfs_dir_get_ino(struct inode *dir, const char *name);

/* -------------------------------------------------------------------------- */
/* ---------- DATA : DIRECTORIES -------------------------------------------- */
/* -------------------------------------------------------------------------- */

int	pnlfs_dir_rm(struct inode *dir, ino_t ino)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (dir == NULL) {
		pr_err("pnlfs_dir_rm() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the entry matching our ino */
	ret = -1;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (le32_to_cpu(rows[i].inode) == ino) {
			strncpy(rows[i].filename, "", PNLFS_FILENAME_LEN);
			rows[i].inode = 0;
			ret = 0;
			pnli->nr_entries--;
			mark_inode_dirty(&pnli->vfs_inode);
			mark_buffer_dirty(bh);
			break;
		}
	}
	brelse(bh);
	return ret;
}

int	pnlfs_dir_add(struct inode *dir, const char *name, ino_t ino)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;

	if (name == NULL) {
		pr_err("pnlfs_dir_add() : NULL parameter 'name'\n");
		return -EINVAL;
	}
	if (name[0] == '\0') {
		pr_err("pnlfs_dir_add() : Empty name\n");
		return -EINVAL;
	}
	if (strnlen(name, PNLFS_FILENAME_LEN) == PNLFS_FILENAME_LEN) {
		pr_err("Filename too long\n");
		return -EINVAL;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_add() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);
	rows = (struct pnlfs_file *)bh->b_data;

	/* If the directory is full, quit */
	if (nr_entries == PNLFS_MAX_DIR_ENTRIES) {
		pr_err("pnlfs_dir_add() : Directory is full\n");
		return -ENOMEM;
	}

	/* Find a free row where we could add our entry */
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // found a free entry
			break;
	}

	/* If the directory is full despite nr_entries < max (shouldnt happen)*/
	if (i == PNLFS_MAX_DIR_ENTRIES) {
		pr_err("pnlfs_dir_add() : No free entries, image corrupted\n");
		return -ENOMEM;
	}

	/* Add our entry */
	rows[i].inode = cpu_to_le32(ino);
	strncpy(rows[i].filename, name, PNLFS_FILENAME_LEN);
	pnli->nr_entries++;
	mark_inode_dirty(&pnli->vfs_inode);
	mark_buffer_dirty(bh);
	brelse(bh);

	return 0;
}

int	pnlfs_dir_set_name(struct inode *dir, ino_t ino, char *src)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (src == NULL) {
		pr_err("pnlfs_dir_set_name() : NULL parameter 'name'\n");
		return -EINVAL;
	}
	if (src[0] == '\0') {
		pr_err("pnlfs_dir_set_name() : Empty name\n");
		return -EINVAL;
	}
	if (strnlen(src, PNLFS_FILENAME_LEN) == PNLFS_FILENAME_LEN) {
		pr_err("Filename too long\n");
		return -EINVAL;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_set_name() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the filename matching our ino */
	ret = -1;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (le32_to_cpu(rows[i].inode) == ino) {
			strncpy(rows[i].filename, src, PNLFS_FILENAME_LEN);
			ret = 0;
			mark_buffer_dirty(bh);
			break;
		}
	}
	brelse(bh);
	return ret;
}

int	pnlfs_dir_get_name(struct inode *dir, ino_t ino, char *dest)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (dest == NULL) {
		pr_err("pnlfs_dir_get_name() : NULL parameter 'name'\n");
		return -EINVAL;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_get_name() : NULL parameter 'dir'\n");
		return -EINVAL;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the filename matching our ino */
	ret = -1;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (le32_to_cpu(rows[i].inode) == ino) {
			strncpy(dest, rows[i].filename, PNLFS_FILENAME_LEN);
			ret = 0;
			break;
		}
	}
	brelse(bh);
	return ret;
}

ino_t	pnlfs_dir_get_ino(struct inode *dir, const char *name)
{
	struct pnlfs_inode_info		*pnli;
	uint32_t			nr_entries;
	sector_t			blkno;
	struct buffer_head		*bh;
	struct pnlfs_file		*rows;
	int				i;
	ino_t				ret;

	if (name == NULL) {
		pr_err("pnlfs_dir_get_ino() : NULL parameter 'name'\n");
		return INO_ERR;
	}
	if (name[0] == '\0') {
		pr_err("pnlfs_dir_get_ino() : Empty name\n");
		return INO_ERR;
	}
	if (strnlen(name, PNLFS_FILENAME_LEN) == PNLFS_FILENAME_LEN) {
		pr_err("Filename too long\n");
		return INO_ERR;
	}
	if (dir == NULL) {
		pr_err("pnlfs_dir_get_ino() : NULL parameter 'dir'\n");
		return INO_ERR;
	}

	/* Get the block where the inode--filename rows are stored */
	pnli = container_of(dir, struct pnlfs_inode_info, vfs_inode);
	nr_entries = pnli->nr_entries;
	blkno = pnli->index_block;
	bh = sb_bread(dir->i_sb, blkno);

	/* Read the block to find the ino matching our filename */
	ret = INO_UNFOUND;
	rows = (struct pnlfs_file *)bh->b_data;
	for (i = 0; i < PNLFS_MAX_DIR_ENTRIES; i++) {
		if ((rows[i].filename)[0] == '\0') // if it's a free entry
			continue;
		if (!strncmp(rows[i].filename, name, PNLFS_FILENAME_LEN)) {
			ret = le32_to_cpu(rows[i].inode);
			break;
		}
	}
	brelse(bh);
	return ret;
}

