#include "runner.h"

Runner::Runner()
{
	_blnIsDying   = false;

	_blnStopping  = false;	

	_enmStatus = RUNNER_STATUS_STOPPED;	
	
	// Creamos el semaforo.	
	_clsSemaphore_p = new Semaphore();

	// Creamos el thread. El thread inicia automaticamente, pero el Runner queda esperando la señal del semaforo. 
	_clsThread_p = new Thread(_ThreadEntry, this);
}

Runner::~Runner()
{
	// Le avisamos al thread que debe morir
	_blnIsDying = true;

	// Señalizamos el semaforo por si el runner esta stopped
	_clsSemaphore_p->Signal();

	// Detener el runner, por si está corriendo
	Stop();

	// Esperamos a que el thread se muera, ¿es necesario?
	_clsThread_p->WaitUntilDeath();

	if (_clsThread_p != NULL)
		delete _clsThread_p;

	if (_clsSemaphore_p != NULL)
		delete _clsSemaphore_p;
}

void* Runner::_ThreadEntry(void* arg)
{
	Runner* owner = (Runner*)arg;

	// Mientras no me esten destruyendo...
	while ( !owner->_blnIsDying ) {

		// Esperamos aqui la señalizacion de un semaforo (Esta señal se emite en 'Start')
		owner->_clsSemaphore_p->Wait();

		// Puede que ahora me esten destruyendo...
		if ( !owner->_blnIsDying ) {

			owner->_enmStatus = RUNNER_STATUS_RUNNING;

			owner->_blnStopping = false;
			
			// Ejecuto una corrida
			owner->_Run();
		}

		owner->_enmStatus = RUNNER_STATUS_STOPPED;
	}

	return NULL;
}

void Runner::Stop()
{
	if (_enmStatus == RUNNER_STATUS_RUNNING) {

		// Seteamos esta bandera para indicar que el Runner debe cancelar la corrida actual
		_blnStopping = true;

		_enmStatus = RUNNER_STATUS_STOPPING;
	}
}

void Runner::Start()
{
	if (_enmStatus == RUNNER_STATUS_RUNNING)
		return;
	
	_clsSemaphore_p->Signal();	
}

RUNNER_STATUS Runner::GetRunStatus() const
{
	return _enmStatus;
}

bool Runner::IsStopping() const
{
	return _blnStopping;
}
