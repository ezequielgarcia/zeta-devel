
#include <math.h>
#include "ion.h"

double DistanceTo(Ion* first, Ion* second)
{
	return sqrt( (first->x-second->x)*(first->x-second->x) + 
		 	     (first->y-second->y)*(first->y-second->y) + 
			     (first->z-second->z)*(first->z-second->z) );
}
