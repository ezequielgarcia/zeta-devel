
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "lattice.h"

static void AllocLattice(double a, size_t count, Lattice* p)
{
	p->lattice_const = a;
	p->ion_count = count;
	p->ions = (Ion*)calloc(count, sizeof(Ion));
}

void FreeLattice(Lattice* p)
{
	free(p->ions);
}

Lattice* GenerateFCC(double lattice_const, size_t side_ion_count)
{
	Lattice* p;
	long c,i,j,k;
	char q;
	double a;
	size_t n;

	q = 1;
	a = lattice_const;
	n = 0;

	p = (Lattice*)calloc(1, sizeof(Lattice));

	c = side_ion_count/2;

	AllocLattice(a, (2*c+1)*(2*c+1)*(2*c+1), p);

	// Generar la red
	for (i=-c; i<=c; i++) {
		for (j=-c; j<=c; j++) { 
			for (k=-c; k<=c; k++) { 

				p->ions[n].q = q;
				p->ions[n].x = i*a;
				p->ions[n].y = j*a;
				p->ions[n].z = k*a;

				// Invertimos la carga 
				q = -q;

				// Siguiente ion
				n++;
			}
		}
	}

	printf("Lattice generated, ion count %d\n", p->ion_count);

	return p;
}

void GenerateBCC()
{	
}

double FastMadelung(Lattice* p)
{
	size_t i,j,center; 
	double d;
	double madelung = 0;
	char q, qq;

	// Buscamos el ion del centro
	for (i=0; i<p->ion_count; i++) {
		if (p->ions[i].x == 0 && p->ions[i].y == 0 && p->ions[i].z == 0) {
			center = i;
			break;
		}
	}
	
	printf("Starting fast madelung calculation (center = %d) ...\n", center);

	q = p->ions[center].q;

	for (j=0; j<p->ion_count; j++) {

		// Salteamos el mismo ion
		if (center==j)
			continue;

		qq = q * p->ions[j].q; 

		// Obtenemos la distancia entre el ion centro y el ion j
		d = DistanceTo( &p->ions[center], &p->ions[j] );

		madelung += qq * (1.0 / d);

		#if 0
		printf(" - %d % (%d	 of  %d) complete...\r", 
				(j+1)*100/vecIons.size(), 
				j, 
				(vecIons.size()-1) );
		fflush(stdout);
		#endif
	}

	printf("\nDone! \n", madelung);

	madelung = madelung * p->lattice_const / q / q ;

	return madelung;
}

double Madelung(Lattice* p)
{
	size_t i,j,center; 
	double d;
	double madelung = 0;
	char q, qq;

	printf("Starting madelung calculation ...\n");

	for (i=0; i<p->ion_count; i++) {

		q = p->ions[i].q;

		for (j=0; j<p->ion_count; j++) {

			// Salteamos el mismo ion
			if (i==j)
				continue;

			qq = q * p->ions[j].q; 

			// Obtenemos la distancia entre el ion centro y el ion j
			d = DistanceTo( &p->ions[i], &p->ions[j] );

			madelung += qq * (1.0 / d);

		}
	}

	printf("\nDone! \n", madelung);

	madelung = madelung * p->lattice_const / q / q / p->ion_count ;

	return madelung;
}


