#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "helloioctl_iface.h"

MODULE_DESCRIPTION("A module for testing ioctl");
MODULE_AUTHOR("Nicolas Phan");
MODULE_LICENSE("GPL");

long	hello_unlocked_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
	char mystr[HELLOIOCTL_MAXLEN] = "Hello ioctl!";
	switch (cmd) {
		case HELLO :
			copy_to_user((char*)arg, mystr, HELLOIOCTL_MAXLEN);
			return 0;
			break;
		default :
			return -ENOTTY;
			break;
	}
}

static struct file_operations fops_helloioctl = {
	.unlocked_ioctl = hello_unlocked_ioctl
	};

static int major;

static int __init helloioctl_init (void)
{
	major = register_chrdev(0, HELLOIOCTL_DEVNAME, &fops_helloioctl);
	pr_info("Helloioctl : Module loaded\n");
	pr_info("Helloioctl : major = %d\n", major);
	return 0;
}

static void __exit helloioctl_exit (void)
{
	unregister_chrdev(major, HELLOIOCTL_DEVNAME);
	pr_info("Helloioctl : Module unloaded\n");
}

module_init(helloioctl_init);
module_exit(helloioctl_exit);
