
#include <sys/ioctl.h>
#include "helloioctl_iface.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main ()
{
	int	fd;
	char	mystr[HELLOIOCTL_MAXLEN] = "Default string\n";

	fd = open("/dev/"HELLOIOCTL_DEVNAME, O_RDONLY);
	if (fd < 0) {
		perror("/dev/"HELLOIOCTL_DEVNAME);
		printf("toto\n");
		return errno;
	}
	if (ioctl(fd, HELLO, mystr) < 0) {
		perror(HELLOIOCTL_DEVNAME" ioctl");
		close(fd);
		return errno;
	}
	printf("%.50s\n", mystr);
	close(fd);
	return 0;
}

