
#include <cmath>
#include "ion.h"

Ion::Ion(char q, double x, double y, double z)
{
	_charge = q;
	_x = x;
	_y = y;
	_z = z;
}

Ion::~Ion()
{
}

char Ion::GetCharge()
{
	return _charge;
}

double Ion::GetX()
{
	return _x;
}

double Ion::GetY()
{
	return _y;
}

double Ion::GetZ()
{
	return _z;
}

double Ion::DistanceTo(Ion& ion)
{
	return sqrt( (ion._x-this->_x)*(ion._x-this->_x) + 
		 	     (ion._y-this->_y)*(ion._y-this->_y) + 
			     (ion._z-this->_z)*(ion._z-this->_z) );
}
