
#include <stdlib.h>
#include <stdio.h>
#include "lattice.h"

int main(int argc, char* argv[])
{
	size_t side_ion_count;
	double madelung; 
	Lattice* lattice;

	if (argc < 2) {
		printf("oops!");
		return 1;
	}

	side_ion_count = atoi(argv[1]);

	// FCC
	// ===	
	printf("Creating lattice...\n");

	lattice = GenerateFCC(1.0, side_ion_count);

	printf("Thinking...\n");

	madelung = Madelung(lattice);

	printf("Madelung constant is %.6f\n", madelung);

}
