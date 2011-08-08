
#include <time.h>
#include "semaphore.h"

Semaphore::Semaphore()
{
	// Creamos un semaforo no-señalizado
	sem_init(&_handle, 0, 0);
}

Semaphore::~Semaphore()
{
	sem_destroy(&_handle);
}

void Semaphore::Wait()
{
	sem_wait(&_handle);
}

bool Semaphore::WaitTimeout(unsigned int uintSeconds)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
        
    ts.tv_sec += uintSeconds;

	return sem_timedwait(&_handle, &ts) == 0 ? true : false;
}

void Semaphore::Signal()
{
	sem_post(&_handle);
}
