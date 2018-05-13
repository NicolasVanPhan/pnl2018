
#ifndef HELLOIOCTL_H
#define HELLOIOCTL_H

#define IOC_DEVNAME	"taskmonitor"
#define IOC_MAGIC	'N'
#define IOC_MAXLEN	50

#define GET_SAMPLE		_IOR(IOC_MAGIC, 0, char*)
#define TASKMON_START		_IOR(IOC_MAGIC, 1, char*)
#define TASKMON_STOP		_IOR(IOC_MAGIC, 2, char*)
#define TASKMON_SET_PID		_IOR(IOC_MAGIC, 3, long)

#endif
