
#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <semaphore.h>

class Semaphore
{

public:
	
	Semaphore();
    ~Semaphore();

	void Wait();
	bool WaitTimeout(unsigned int uintSeconds);
	void Signal();

private:

	sem_t _handle;
   
};

#endif
