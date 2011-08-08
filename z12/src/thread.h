#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>

class Thread
{

public:
	
	Thread(void* (*start_routine)(void*), void* arg);
    ~Thread();

	void WaitUntilDeath();

private:
    pthread_t _handle;
   
};

#endif
