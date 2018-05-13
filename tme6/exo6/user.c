
#include <sys/ioctl.h>
#include "taskmonitor.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void	disp_usage(void)
{
	printf("Usage: user [option]\n\
	OPTIONS:\n\
	  g : Display a sample (GET_SAMPLE ioctl)\n\
	  s : Start periodic syslog write (TASKMON_START)\n\
	  S : Stop periodic syslog write (TASKMON_STOP)\n\
	  p [pid] : Change monitored task (TASKMON_SET_PID)\n");
}

int main (int argc, char* argv[])
{
	int	fd;
	char	cmd;
	int	pid;
	long	ioctl_cmd;
	long	ioctl_arg;
	char	mystr[IOC_MAXLEN] = "Default string\n";

	/* Retrieve user command */
	if (argc < 2 || strlen(argv[1]) != 1)
		goto abort;
	cmd = argv[1][0];
	switch (cmd) {
		case 'p' :
			if (argc < 3)
				goto abort;
			pid = atoi(argv[2]);
			if (pid == 0)
				goto abort;
			ioctl_cmd = TASKMON_SET_PID;
			break;
		case 'g' :
			ioctl_cmd = GET_SAMPLE;
			break;
		case 's' :
			ioctl_cmd = TASKMON_START;
			break;
		case 'S' :
			ioctl_cmd = TASKMON_STOP;
			break;
		default :
			goto abort;
			break;
	}

	/* Open device */
	fd = open("/dev/"IOC_DEVNAME, O_RDONLY);
	if (fd < 0) {
		perror("/dev/"IOC_DEVNAME);
		printf("toto\n");
		return errno;
	}

	/* Process user command */
	ioctl_arg = (long)mystr;
	if (ioctl_cmd == TASKMON_SET_PID)
		ioctl_arg = pid;
	if (ioctl(fd, ioctl_cmd, ioctl_arg) < 0) {
		perror(IOC_DEVNAME" ioctl");
		close(fd);
		return errno;
	}
	if (ioctl_cmd == GET_SAMPLE)
		printf("%.*s\n", IOC_MAXLEN, mystr);

	close(fd);
	return 0;
abort:
	disp_usage();
	return -1;
}

