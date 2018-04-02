#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

MODULE_DESCRIPTION("Module qui affiche des infos sur les superblocks");
MODULE_AUTHOR("Nicolas Phan");
MODULE_LICENSE("GPL");

static void disp_sb(struct super_block *sb, void *others);

static int __init my_init(void)
{
	pr_info("Load module display_sb\n");
	iterate_supers(&disp_sb, NULL);
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
	/* int	offset; */
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
	pr_info("uuid=%s type=%.*s\n", str_uuid, 10, sb->s_type->name);
}
 
