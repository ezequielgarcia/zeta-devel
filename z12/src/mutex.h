
#ifndef _MT_MUTEX_H
#define _MT_MUTEX_H

#include <pthread.h>

class Mutex {
	
	pthread_mutex_t	_mutex;
	
public:

	// Constructor
	Mutex();

	// Destructor
	~Mutex();

	// Toma el mutex
	void Lock();

	// Libera el mutex
	void Unlock();

};

#endif
