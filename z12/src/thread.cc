#include "thread.h"

Thread::Thread(void *(*start_routine)(void*), void* arg)
{
	pthread_attr_t attr;
	
	// Inicializamos los atributos
	pthread_attr_init(&attr);
   	
	// Esta recomendado seteardd explicitamente los threads como 'joinable'
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// Creamos y lanzamos el thread
	pthread_create(&_handle, &attr, start_routine, arg);

	// Destruimos los atributos
	pthread_attr_destroy(&attr);
}

Thread::~Thread()
{
	WaitUntilDeath();	
}

void Thread::WaitUntilDeath()
{
	// Esperamos hasta que el thread termine 
	pthread_join(_handle, NULL);
}
