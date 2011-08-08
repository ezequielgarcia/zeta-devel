
#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <string>

using namespace std;

class Serial {

private:

	string	_strDevicePath;
	string	_strDeviceName;

	int		_fd;

	size_t	_szMinBytes;

public:

	Serial(const string& path, size_t min_bytes);
	~Serial();

	bool Open();
	void Close();

	ssize_t Read(unsigned char* buffer, size_t bytes);
};

#endif
