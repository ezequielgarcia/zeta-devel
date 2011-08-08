
#ifndef _Z12_MANAGER_H_
#define _Z12_MANAGER_H_

#include "runner.h"
#include "safe_buffer.h"
#include "z12_preprocessor.h"

class Z12Manager : public Runner {

private:

	Semaphore _sem;

	SafeBuffer& _buffer;

	Z12Preprocessor _clsPreproc;

protected: 

	void _Run();

public:

    Z12Manager(SafeBuffer& buffer);
	~Z12Manager();

	void WaitUntilStop();
};

#endif
