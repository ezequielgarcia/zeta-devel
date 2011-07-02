
#include <stdlib.h>
#include <stdio.h>
#include "lattice.h"

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("oops!");
		return 1;
	}

	float 	madelung; 
	Lattice lattice;

	int count = atoi(argv[1]);

	// FCC
	// ===	
	printf("Creating lattice...\n");

	lattice.GenerateFCC(count);

	printf("Thinking...\n");

	madelung = lattice.FastMadelung();

	printf("Madelung constant is %.6f\n", madelung);

}
