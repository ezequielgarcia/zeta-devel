
#ifndef _ION_H_
#define _ION_H_

typedef struct __Ion 
{
	double x, y, z;
	char   q;
} Ion ;

double DistanceTo(Ion*, Ion*);

#endif
