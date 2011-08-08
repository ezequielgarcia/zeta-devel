
#ifndef _SERIAL_MANAGER_H_
#define _SERIAL_MANAGER_H_

#include "runner.h"
#include "serial.h"
#include "safe_buffer.h"

class SerialManager : public Runner {

private:

	Serial	_clsSerial;

	Semaphore _sem;

	SafeBuffer& _buffer;

protected: 

	void _Run();

public:

    SerialManager(const string& dev, size_t min_bytes, SafeBuffer& buffer);
	~SerialManager();

	// We override Runner::Start()
	bool Start();

	void WaitUntilStop();
};

#endif
