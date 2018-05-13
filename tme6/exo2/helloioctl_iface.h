
#ifndef HELLOIOCTL_H
#define HELLOIOCTL_H

#define HIOC_DEVNAME	"hello"
#define HIOC_MAGIC	'N'
#define HIOC_MAXLEN	50
#define HIOC_NMAXLEN	50
#define HELLO			_IOR(HIOC_MAGIC, 0, char*)
#define NAME			_IOR(HIOC_MAGIC, 1, char*)

#endif
