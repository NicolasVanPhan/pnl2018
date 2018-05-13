
#ifndef HELLOIOCTL_H
#define HELLOIOCTL_H

#define HELLOIOCTL_DEVNAME	"hello"
#define HELLOIOCTL_MAGIC	'N'
#define HELLOIOCTL_MAXLEN	50
#define HELLO			_IOR(HELLOIOCTL_MAGIC, 0, char*)

#endif
