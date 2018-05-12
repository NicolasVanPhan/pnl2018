#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>

#define WORD_MAXLEN	30

MODULE_DESCRIPTION("A module for testing sysfs");
MODULE_AUTHOR("Nicolas Phan <nicolas.van.phan@gmail.com>");
MODULE_LICENSE("GPL");

static char word[WORD_MAXLEN] = "sysfs";

static ssize_t hello_show (struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	return sprintf(buf, "Hello %.*s !\n", WORD_MAXLEN, word);
}

static ssize_t hello_store (struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	strncpy(word, buf, WORD_MAXLEN);
	return count;
}

static struct kobj_attribute	hello = __ATTR_RW(hello);

static int hello_init(void)
{
	sysfs_create_file(kernel_kobj, &(hello.attr));
	pr_warn("Hello sysfs module loaded\n");
	return 0;
}

static void hello_exit(void)
{
	sysfs_remove_file(kernel_kobj, &(hello.attr));
	pr_warn("Hello sysfs module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);
