#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

MODULE_DESCRIPTION("Module qui affiche les superblocks d'un type particulier");
MODULE_AUTHOR("Nicolas Phan");
MODULE_LICENSE("GPL");

char	*fs = "ext4";
module_param(fs, charp, 0);
MODULE_PARM_DESC(fs, "The file system type of the partitions to display");

static void disp_sb(struct super_block *sb, void *others);

static int __init my_init(void)
{
	struct file_system_type *fstype;

	pr_info("Load module display_sb\n");
	fstype = get_fs_type(fs);
	iterate_supers_type(fstype, &disp_sb, NULL);

	return 0;
}
module_init(my_init);

static void __exit my_exit(void)
{
	pr_info("Unload module display_sb\n");
}
module_exit(my_exit);

/*
 * disp_sb    -    Display a superblock 
 * @s : The superblock to display
 *
 * disp_sb() prints various informations about a given superblock
 */
static void disp_sb(struct super_block *sb, void *others)
{
	int	i;
	char	str_uuid[37];
	int	cursor;

	/* Convert uuid into string */
	cursor = 0;
	for (i = 0; i < 16; i++) {
		if (i == 4 || i == 6 || i == 8 || i == 10) {
			scnprintf(&str_uuid[cursor], 2, "-");
			cursor++;
		}
		scnprintf(&str_uuid[cursor], 3, "%02x", sb->s_uuid[i]);
		cursor += 2;
	}

	/* Display the superblock */
	pr_info("uuid=%s type=%.*s time=%ld.%ld\n",
		str_uuid,
		10,
		sb->s_type->name,
		sb->s_last_disp.tv_sec,
		sb->s_last_disp.tv_nsec);

	/* Update the last-display time */
	getnstimeofday(&sb->s_last_disp);
}
