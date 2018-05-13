#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "helloioctl_iface.h"

MODULE_DESCRIPTION("A module for testing ioctl");
MODULE_AUTHOR("Nicolas Phan");
MODULE_LICENSE("GPL");

char name[HIOC_NMAXLEN] = "Nicolas";

long	hello_unlocked_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
	char mystr[HIOC_MAXLEN];
	scnprintf(mystr, HIOC_MAXLEN, "Hello %.*s", HIOC_MAXLEN - 20, name);
	switch (cmd) {
		case HELLO :
			copy_to_user((char*)arg, mystr, HIOC_MAXLEN);
			return 0;
			break;
		case NAME :
			copy_from_user(name, (char*)arg, HIOC_NMAXLEN);
			name[HIOC_NMAXLEN - 1] = '\0';
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
	major = register_chrdev(0, HIOC_DEVNAME, &fops_helloioctl);
	pr_info("Helloioctl : Module loaded\n");
	pr_info("Helloioctl : major = %d\n", major);
	return 0;
}

static void __exit helloioctl_exit (void)
{
	unregister_chrdev(major, HIOC_DEVNAME);
	pr_info("Helloioctl : Module unloaded\n");
}

module_init(helloioctl_init);
module_exit(helloioctl_exit);
