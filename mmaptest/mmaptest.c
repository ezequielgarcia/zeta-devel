#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SIZE 4096*4

int main(int argc, char* argv[])
{
	int fd = 0;

	if (argc < 2) {
		printf("Need a device\n");
		return 1;
	}

	// Open the file for reading and writing 
	fd = open(argv[1], O_RDWR);
	if (!fd) {
		printf("Error: cannot open mmap device.\n");
		return 1;
	}
	printf("The mmap device was opened successfully.\n");

	// Map the device to memory
	void* p = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);       
	if ((long)p == -1) { 
		printf("Error: failed to map device to memory.\n"); 
		return 1;
	}
	printf("The device was mapped to memory successfully, %p.\n", p);

	// Set to something
	memset(p, 0x80, SIZE);

	getchar();

	munmap(p, SIZE);

	close(fd);

	return 0;
}
