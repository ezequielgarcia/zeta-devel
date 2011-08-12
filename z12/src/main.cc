#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "serial_manager.h"
#include "z12_manager.h"

int main(int argc, char* argv[])
{
	unsigned char buff[32];
	SafeBuffer buffer;
	Z12Manager z_man(buffer);

	z_man.Start();

	printf("Z12 Manager starting...\n");
	sleep(1);

#if 0
	SerialManager s_man(argv[1], 1, buffer);

	if ( !s_man.Start() ) {
		printf("Serial manager failed to start, exiting...\n");
		return EXIT_FAILURE;
	}	

	printf("Serial Manager starting...\n");
	sleep(1);

	s_man.WaitUntilStop();

	printf("Serial Manager Stopped\n");


#else
	FILE* fp = fopen(argv[1], "r");

	size_t r;
	int count = 0;
	while (!feof(fp)) {
		r = fread(buff, 1, 1, fp);
		buffer.Push(buff[0]);

		#if 0
		count++;
		printf("%02hhx ", buff[0]);
		if (count == 16) {
			printf("\n");
			count = 0;
		}
		fflush(stdout);
		#endif
	}

	fclose(fp);
#endif

	z_man.Stop();
	z_man.WaitUntilStop();

	printf("Z12 Manager Stopped\n");

	return EXIT_SUCCESS;
}
