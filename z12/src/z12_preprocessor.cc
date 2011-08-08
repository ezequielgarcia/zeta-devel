
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

		;
	}
	else {
		printf("Wrong header!\n");
	}
}

void Z12Preprocessor::Push(unsigned char c)
{
	// Detect end of message
	
	// Is LineFeed?
	if (c == 0x0A) {
	

		// Had CarriageReturn before?
		if (_blnCR) {

			printf("CRLF detected. Building a message...\n");
		
			// Build current message and flush list
			_BuildMessage();
			_listRaw.clear();

			// Lower flag for next message
			_blnCR = false;
		}

		// Probably broken message
		printf("LF without CR detected, flushing ...\n");
		_listRaw.clear();
	}
	else if (c == 0x0D) {
	
		printf("CR detected\n");
		_blnCR = true;
	}
	else {

		// Push into raw list
		_listRaw.push_back(c);
	}
}
