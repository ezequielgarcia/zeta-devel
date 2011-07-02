/*
 *  felu.c - the process to use ioctl's to control the kernel module
 *
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 */

/* 
 * device specifics, such as ioctl numbers and the
 * major device file. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>		/* ioctl */
#include "../felk.h"

/* 
 * Main - Call the ioctl functions 
 */
main()
{
	int fd, ret;

	fd = open(DEVICE_FILE_NAME, 0);
	if (fd < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	ret = ioctl(fd, IOCTL_FELK_HELLO, NULL);
	if (ret < 0) {
		printf("Can't execute ioctl: %d\n", ret);
	}

	close(fd);
}
