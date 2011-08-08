
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "serial.h"

// TODO: Poner como parametro de construccion
#define BAUD_RATE	B115200

Serial::Serial(const string& path, size_t min_bytes)
{
	_strDevicePath = path;
	_fd = -1;
	_szMinBytes = min_bytes;
}

Serial::~Serial()
{
	// Guarantee the device gets closed
	Close();
}

bool Serial::Open()
{
	struct termios config;

	/**
	 * O_RDWR: Opens the port for read only
	 * O_NOCTTY: The port never becomes the controlling terminal of the process.
	 * O_NDELAY: Use non-blocking I/O. On some systems this also means the RS232 DCD signal line is ignored.
	 */
	_fd = open(_strDevicePath.c_str(), O_RDONLY | O_NOCTTY | O_NDELAY);
	if (_fd == -1) {
		printf("Failed to open device\n" );
		return false;
	}

	if (isatty(_fd) != 1) { 
		printf("Device is not a serial tty\n");
		return false;
	}

	// obtain device name
	_strDeviceName = ttyname(_fd);

	if (tcgetattr(_fd, &config) < 0) {
		printf("Cant get terminal attributes\n");
		return false;
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
	config.c_cc[VMIN]  = _szMinBytes;
	config.c_cc[VTIME] = 0;

	//
	// Communication speed (simple version, using the predefined
	// constants)
	//
	if (cfsetispeed(&config, BAUD_RATE) < 0 || 
		cfsetospeed(&config, BAUD_RATE) < 0) {

		printf("Cant set terminal baud rate\n");
		return false;
	}

	//
	// Finally, apply the configuration
	//
	if (tcsetattr(_fd, TCSAFLUSH, &config) < 0) {
		printf("Cant set terminal attributes\n");
		return false;
	}

	printf("Device '%s' opened on fd %d\n", _strDeviceName.c_str(), _fd);

	return true;
}

void Serial::Close()
{
	if (_fd > 0)
		close(_fd);
}

ssize_t Serial::Read(unsigned char* buffer, size_t bytes)
{
	if (_fd > 0) 
		return read(_fd, buffer, bytes);	

	printf("Device not opened, cant read!\n");
	return 0;
}
