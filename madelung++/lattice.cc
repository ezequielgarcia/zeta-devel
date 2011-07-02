
#include <stdio.h>
#include <math.h>
#include "lattice.h"

// a es la distancia entre primeros vecinos
#define a 1

Lattice::Lattice()
{
}

Lattice::~Lattice()
{
	if ( !vecIons.empty() ) 
		Clean();	
}

void Lattice::Clean()
{
	// Destruir los objetos de la red
	while ( !vecIons.empty() ) {
		delete vecIons.back();
		vecIons.pop_back();	
	}
}

void Lattice::GenerateFCC(int count)
{
	char q = 1;

	// Generar la red
	for (int i=-count; i<=count; i++) {
		for (int j=-count; j<=count; j++) { 
			for (int k=-count; k<=count; k++) { 

				vecIons.push_back( new Ion(q, i*a, j*a, k*a) );

				// Invertimos la carga 
				q = -q;
			}
		}
	}

	/*
	char q;

	if ( !vecIons.empty() ) 
		Clean();	

	q = 1;
	// Generar la red de bravais cubica
	for (int i=-count; i<=count; i++) {
		for (int j=-count; j<=count; j++) { 
			for (int k=-count; k<=count; k++) { 

				vecIons.push_back( new Ion(q, (j+k)*2*a, (i+k)*2*a, (i+j)*2*a) );
			}
		}
	}

	q = -1;
	// Generar el motivo
	for (int i=-count; i<=count; i++) {
		for (int j=-count; j<=count; j++) { 
			for (int k=-count; k<=count; k++) { 

				vecIons.push_back( new Ion(q, (j+k+0.5)*2*a, (i+k)*2*a, (i+j)*2*a) );
			}
		}
	}
	*/

	printf("Lattice generated, ion count %d\n", vecIons.size());
}

void Lattice::GenerateBCC(int count)
{	
	#if 0
	char q;

	double r = 2.0 * a / sqrt(3);

	if ( !vecIons.empty() ) 
		Clean();	

	q = 1;
	// Generar la red de bravais cubica
	for (int i=-count; i<=count; i++) {
		for (int j=-count; j<=count; j++) { 
			for (int k=-count; k<=count; k++) { 

				vecIons.push_back( new Ion(q, i*r, j*r, k*r) );
			}
		}
	}

	q = -1;
	// Generar el motivo
	for (int i=-count; i<=count; i++) {
		for (int j=-count; j<=count; j++) { 
			for (int k=-count; k<=count; k++) { 

				vecIons.push_back( new Ion(q, (i+0.5)*r, (j+0.5)*r, (k+0.5)*r) );
			}
		}
	}

	printf("Lattice generated, ion count %d\n", vecIons.size());
	#endif
}

double Lattice::FastMadelung()
{
	size_t center; 
	double d;
	double madelung = 0;
	char q, qq;

	// Buscamos el ion del centro
	for (int i=0; i<vecIons.size(); i++) {
		if (vecIons[i]->GetX() == 0 && vecIons[i]->GetY() == 0 && vecIons[i]->GetZ() == 0) {
			center = i;
			break;
		}
	}

	printf("Starting fast madelung calculation (center = %d) ...\n", center);

	q = vecIons[center]->GetCharge(); 

	for (int j=0; j<vecIons.size(); j++) {

		// Salteamos el mismo ion
		if (center==j)
			continue;

		qq = q * vecIons[j]->GetCharge(); 

		// Obtenemos la distancia entre el ion i y el ion j
		d = vecIons[center]->DistanceTo( *vecIons[j] );

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

	madelung = madelung * a ;

	return madelung;
}

double Lattice::Madelung()
{
	double d;
	double madelung = 0;
	char q, qq;

	printf("Starting madelung calculation...\n");

	// Para cada ion de la red
	for (int i=0; i<vecIons.size(); i++) {
	
		q = vecIons[i]->GetCharge(); 
	
		for (int j=i; j<vecIons.size(); j++) {

			// Salteamos el mismo ion
			if (i==j)
				continue;
		
			qq = q * vecIons[j]->GetCharge(); 
		
			// Obtenemos la distancia entre el ion i y el ion j
			d = vecIons[i]->DistanceTo( *vecIons[j] );

			madelung += qq * (1.0 / d);

			#if 0
			printf(" - %d % (%d	 of  %d) complete...\r", 
						(i+1)*100/vecIons.size(), 
						(i+1)*(j+1), 
						(vecIons.size()-1) * vecIons.size() );
			fflush(stdout);
			#endif
		}
	}		

	printf("\nDone! \n", madelung);

	madelung = madelung * a * 2 / vecIons.size() / q / q ;

	return madelung;
}

