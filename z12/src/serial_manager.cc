
#include <stdio.h>
#include "serial_manager.h"

#define BUFF_SIZE 1024

SerialManager::SerialManager(const string& dev, size_t min_bytes)
	: _clsSerial(dev, min_bytes)
{
}

SerialManager::~SerialManager()
{
	Runner::Stop();
}

void SerialManager::_Run()
{
	unsigned char buffer[BUFF_SIZE];
	size_t count = 0;
	ssize_t c;

	while ( !IsStopping() ) {

		c = _clsSerial.Read(buffer, 1);	

		// For testing only
		if (c > 1)
			printf("Oops! Read %d bytes! more byte than asked for!\n", c);

		if (c == -1) {
			perror("Read");
			break;
		}

		#if 0
		count++;
		printf("%hhx ", (unsigned char)buffer[0]);

		if (count == 16) {
			printf("\n");
			count = 0;
		}
		#else
		// printf("%c", buffer[0]);
		write(0, buffer, 1);
		#endif
	}

	_clsSerial.Close();

	_sem.Signal();
}

bool SerialManager::Start()
{
	// try to open serial device
	if ( !_clsSerial.Open() ) 
		return false;
	
	// if opened OK start the runner
	Runner::Start();

	return true;
}

void SerialManager::WaitUntilStop()
{
	if (GetRunStatus() == RUNNER_STATUS_RUNNING)
		_sem.Wait();
}
