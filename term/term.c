
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
	int fd = -1;
	struct termios config;
	const char* device_path;
	unsigned char buffer[32];

	if (argc < 2)
		printf("Need a device!\n");

	device_path = argv[1];

	printf("Opening %s ...\n", device_path);

	/**
	 * O_RDWR: Opens the port for reading and writing
	 * O_NOCTTY: The port never becomes the controlling terminal of the process.
	 * O_NDELAY: Use non-blocking I/O. On some systems this also means the RS232 DCD signal line is ignored.
	 */
	fd = open(device_path, O_RDWR | O_NOCTTY | O_NDELAY);
	if(fd == -1) {
		printf("Failed to open device\n" );
		exit(EXIT_FAILURE);
	}

	if (isatty(fd) != 1) { 
		printf("Device is not serial tty\n");
		exit(EXIT_FAILURE);
	}

	printf("TTY '%s' opened OK\n", tty_name(fd));	

	if (tcgetattr(fd, &config) < 0) {
		printf("Cant get terminal attributes\n");
		exit(EXIT_FAILURE);
	}

	//
	// Input flags - Turn off input processing
	// convert break to null byte, no CR to NL translation,
	// no NL to CR translation, don't mark parity errors or breaks
	// no input parity check, don't strip high bit off,
	// no XON/XOFF software flow control
	//
	config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
			INLCR | PARMRK | INPCK | ISTRIP | IXON | IXOFF);
	//
	// Output flags - Turn off output processing
	// no CR to NL translation, no NL to CR-NL translation,
	// no NL to CR translation, no column 0 CR suppression,
	// no Ctrl-D suppression, no fill characters, no case mapping,
	// no local output processing
	//
	// config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
	//                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
	config.c_oflag = 0;
	//
	// No line processing:
	// echo off, echo newline off, canonical mode off, 
	// extended input processing off, signal chars off
	//
	config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	//
	// Turn off character processing
	// clear current char size mask, no parity checking,
	// no output processing, force 8 bit input
	//
	config.c_cflag &= ~(CSIZE | PARENB);
	config.c_cflag |= CS8;
	//
	// One input byte is enough to return from read()
	// Inter-character timer off
	//
	config.c_cc[VMIN]  = 1;
	config.c_cc[VTIME] = 0;
	//
	// Communication speed (simple version, using the predefined
	// constants)
	//
	if(	cfsetispeed(&config, B115200) < 0 || 
		cfsetospeed(&config, B115200) < 0) {

		printf("Cant set terminal baud rate\n");
		exit(EXIT_FAILURE);
	}

	//
	// Finally, apply the configuration
	//
	if( tcsetattr(fd, TCSAFLUSH, &config) < 0) {
		printf("Cant set terminal attributes\n");
		exit(EXIT_FAILURE);
	}

	printf("Reading from terminal ...\n");
	int count = 0;
	while (1) {

		// Read 1 byte, blocks right? 
		if (read(fd,&buffer,1) > 0) {
	
			count++;
			printf("%hhx ", (unsigned char)buffer);

			if (count == 16) {
				printf("\n");
				count = 0
			}
		}
		else {
			printf("No data in tty. Should never happen!\n");	
		}
	}

	close(fd);

	return EXIT_SUCCESS;
}
