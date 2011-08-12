
#include <stdio.h>
#include "z12_preprocessor.h"

Z12Preprocessor::Z12Preprocessor()
	: _blnCR(false)
{
}

Z12Preprocessor::~Z12Preprocessor()
{
}

void Z12Preprocessor::_BuildMessage()
{
	list<unsigned char>::iterator it = _listRaw.begin();
	unsigned char header[12];

	if (_listRaw.size() < 12) {
		printf("Discarded!\n");
		_listRaw.clear();
		return;
	}

	for (size_t j=0; j<12; j++) {

		header[j] = *it;	
		it++;

		if (it == _listRaw.end())
			break;
	}

	// Detect header
	if (header[0] == '$' &&
		header[1] == 'P' &&
		header[2] == 'A' &&
		header[3] == 'S' &&
		header[4] == 'H' &&
		header[5] == 'R') {

		printf("Msg type %c%c%c\n", header[7], header[8], header[9]);
	}
	else {
		printf("!!! ERROR: Wrong header !!!\n");
	}
}

void Z12Preprocessor::Push(unsigned char c)
{
	// Try to detect end of message:
	// Is LineFeed?
	if (c == 0x0A) {
	
		// Had CarriageReturn before?
		if (_blnCR) {

			// Build current message and flush list
			_BuildMessage();
			_listRaw.clear();

			// Lower flag for next message
			_blnCR = false;
		
			return;
		}
	}
	else if (c == 0x0D) {
	
		_blnCR = true;
	}
	else {
		// If we had CR and this is not LF, we lower flag
		if (_blnCR)
			_blnCR = false;
	}

	// Push into raw list
	_listRaw.push_back(c);
}
