
#include <sys/ioctl.h>
#include "helloioctl_iface.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main ()
{
	int	fd;
	char	mystr[HIOC_MAXLEN] = "Default string\n";

	fd = open("/dev/"HIOC_DEVNAME, O_RDONLY);

	/* Open device */
	if (fd < 0) {
		perror("/dev/"HIOC_DEVNAME);
		printf("toto\n");
		return errno;
	}

	/* Display message */
	if (ioctl(fd, HELLO, mystr) < 0) {
		perror(HIOC_DEVNAME" ioctl");
		close(fd);
		return errno;
	}
	printf("%.50s\n", mystr);

	/* Change name */
	snprintf(mystr, 5, "Toto");
	if (ioctl(fd, NAME, mystr) < 0) {
		perror(HIOC_DEVNAME" ioctl");
		close(fd);
		return errno;
	}

	/* Display message */
	if (ioctl(fd, HELLO, mystr) < 0) {
		perror(HIOC_DEVNAME" ioctl");
		close(fd);
		return errno;
	}
	printf("%.50s\n", mystr);

	close(fd);
	return 0;
}

