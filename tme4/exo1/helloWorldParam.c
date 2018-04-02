#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Module \"hello word param\" pour noyau linux");
MODULE_AUTHOR("Julien Sopena, LIP6");
MODULE_LICENSE("GPL");

static char	*whom = "John";
module_param(whom, charp, 0644);
MODULE_PARM_DESC(whom, "To whom shall the program say hello");

static int	howmany = 0;
module_param(howmany, int, 0644);
MODULE_PARM_DESC(howmany, "How many times shall the program say hello");

static int	maxlen = 20;
static int	maxoccur = 10;

static int __init hello_init(void)
{
	int	i;

	howmany = howmany < maxoccur ? howmany : maxoccur;
	for (i = 0; i < howmany; i++) {
		pr_info("(%d) Hello, %.*s\n", i, maxlen, whom);
	}
	return 0;
}
module_init(hello_init);

static void __exit hello_exit(void)
{
	pr_info("Goodbye, %.*s\n", maxlen, whom);
}
module_exit(hello_exit);

