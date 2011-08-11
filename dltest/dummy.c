
#include <stdio.h>
#include "loaded.h"

void __attribute__ ((constructor)) dummy_enter()
{
	printf("Constructing dummy ...\n");

	loaded();
}

void __attribute__ ((destructor)) dummy_exit()
{
	printf("Destroying dummy ...\n");
}
