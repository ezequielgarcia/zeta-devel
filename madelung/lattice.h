
#ifndef _LATTICE_H_
#define _LATTICE_H_

#include "ion.h"

typedef struct __Lattice 
{
	double lattice_const;
	size_t ion_count;
	Ion*   ions;
} Lattice ;
	
Lattice* GenerateFCC(double lattice_const, size_t side_ion_count);
void FreeLattice(Lattice*);

double Madelung(Lattice*);
double FastMadelung(Lattice*);

#endif
