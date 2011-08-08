
#ifndef _Z12_PREPROCESSOR_H_
#define _Z12_PREPROCESSOR_H_

#include <list>

using namespace std;

enum Z12_MSG_TYPE {

	Z12_MSG_MPC,
	Z12_MSG_PBN,
	Z12_MSG_OTHER,
};

class Z12Preprocessor {

private:

	bool _blnCR;
	
	list<Z12_MSG_TYPE> _listMsg;

	list<unsigned char> _listRaw;

	void _BuildMessage();

public:

	Z12Preprocessor();
	~Z12Preprocessor();

	void Push(unsigned char c);
};

#endif
