
#ifndef _LATTICE_H_
#define _LATTICE_H_

#include <vector>
#include "ion.h"

using namespace std;

class Lattice {
	
private:

	vector<Ion*> vecIons;

public:

	Lattice();
	~Lattice();

	void Clean();

	void GenerateFCC(int);
	void GenerateBCC(int);

	double Madelung();
	double FastMadelung();
};

#endif
