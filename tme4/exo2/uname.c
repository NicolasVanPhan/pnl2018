#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>

MODULE_DESCRIPTION("Module qui renomme le noyau");
MODULE_AUTHOR("Nicolas Phan");
MODULE_LICENSE("GPL");

static char	*name = "Anonymous";
module_param(name, charp, 0);
MODULE_PARM_DESC(name, "The new kernel name to set");
static int	len;
static char	old_name[__NEW_UTS_LEN + 1];

static int __init hello_init(void)
{
	int	i;

	pr_info("Load uname module\n");

	/* Save old kernel name */
	pr_info("Old kernel name : %s\n", init_uts_ns.name.sysname);
	for (i = 0; i <= __NEW_UTS_LEN; i++)
		old_name[i] = init_uts_ns.name.sysname[i];

	/* Set new kernel name */
	len = strnlen(name, __NEW_UTS_LEN);
	for (i = 0; i < len; i++) {
		init_uts_ns.name.sysname[i] = name[i];
	}
	init_uts_ns.name.sysname[i] ='\0';
	pr_info("New kernel name : %s\n", init_uts_ns.name.sysname);

	return 0;
}
module_init(hello_init);

static void __exit hello_exit(void)
{
	int	i;

	/* Restore old kernel name */
	for (i = 0; i <= __NEW_UTS_LEN; i++)
		init_uts_ns.name.sysname[i] = old_name[i];

	pr_info("Unload uname module\n");
}
module_exit(hello_exit);

