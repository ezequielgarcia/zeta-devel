#include "mutex.h"

Mutex::Mutex()
{
	// Crear mutex
	pthread_mutex_init(&_mutex, NULL);
}

Mutex::~Mutex()
{
	// Eliminar mutex
	pthread_mutex_destroy(&_mutex);
}

// Toma el mutex
void Mutex::Lock()
{
	pthread_mutex_lock(&_mutex);
}

// Libera el mutex
void Mutex::Unlock()
{
	pthread_mutex_unlock(&_mutex);
}

