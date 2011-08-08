#include <stdio.h>
#include <stdlib.h>
#include "serial_manager.h"

int main(int argc, char* argv[])
{
	SerialManager manager(argv[1], 1);

	printf("Serial Manager starting...\n");

	if ( !manager.Start() )
		return EXIT_FAILURE;	

	manager.WaitUntilStop();

	printf("Serial Manager Stopped\n");

	return EXIT_SUCCESS;
}
