#include <stdio.h>
#include <stdlib.h>
#include "serial_manager.h"

int main(int argc, char* argv[])
{
	SerialManager manager(argv[1], 1);

	if ( !manager.Start() ) {
		printf("Serial manager failed to start, exiting...\n");
		return EXIT_FAILURE;
	}	

	printf("Serial Manager starting...\n");
	sleep(1);

	manager.WaitUntilStop();

	printf("Serial Manager Stopped\n");

	return EXIT_SUCCESS;
}
