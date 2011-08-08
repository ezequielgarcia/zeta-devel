#include "runner.h"

Runner::Runner()
	: _clsThread(_ThreadEntry, this)
{
	_blnStopping  = false;	

	_enmStatus = RUNNER_STATUS_STOPPED;	
}

Runner::~Runner()
{
	_enmStatus = RUNNER_STATUS_DESTROYING;
	_blnStopping = true;

	// Signal semaphore in case it is running
	_clsSemStart.Signal();

	// Wait until thread gets really killed
	_clsThread.WaitUntilDeath();
}

void* Runner::_ThreadEntry(void* arg)
{
	Runner* owner = (Runner*)arg;

	// Wait until runner gets started
	owner->_clsSemStart.Wait();

	// Thread may get stopped before running
	if (owner->_enmStatus == RUNNER_STATUS_DESTROYING)
		return NULL;

	owner->_enmStatus = RUNNER_STATUS_RUNNING;

	owner->_blnStopping = false;
			
	// Ejecuto una corrida
	owner->_Run();

	owner->_enmStatus = RUNNER_STATUS_STOPPED;

	return NULL;
}

void Runner::Stop()
{
	if (_enmStatus == RUNNER_STATUS_RUNNING) {

		// Set this flag to indicate the runner must stop, this must 
		// be checked inside _Run implementation using IsStopping().
		_blnStopping = true;

		_enmStatus = RUNNER_STATUS_STOPPING;
	}
}

void Runner::Start()
{
	if (_enmStatus == RUNNER_STATUS_RUNNING)
		return;
	
	_clsSemStart.Signal();	
}

RUNNER_STATUS Runner::GetRunStatus() const
{
	return _enmStatus;
}

bool Runner::IsStopping() const
{
	return _blnStopping;
}
