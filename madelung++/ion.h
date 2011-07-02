
#ifndef _ION_H_
#define _ION_H_

class Ion {
	
private:

	double _x, _y, _z;
	char  _charge;

public:

	Ion(char q, double x, double y, double z);
	~Ion();

	double GetX();
	double GetY();
	double GetZ();

	double DistanceTo(Ion& ion);
	char GetCharge();
};

#endif
