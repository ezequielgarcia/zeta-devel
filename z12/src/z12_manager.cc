
#include <stdio.h>
#include <unistd.h>
#include "z12_manager.h"

#define BUFF_SIZE 1024

Z12Manager::Z12Manager(SafeBuffer& buffer)
	: _buffer(buffer)
{
}

Z12Manager::~Z12Manager()
{
	Runner::Stop(); 
}

void Z12Manager::_Run() { 

	unsigned char c;

	while ( !IsStopping() ) {
			
		if ( _buffer.Empty() )
			usleep(1000);

		while ( !_buffer.Empty() ) {
			c = _buffer.Pop();
			_clsPreproc.Push(c);
		}
	}

	while ( !_buffer.Empty() ) {
		c = _buffer.Pop();
		_clsPreproc.Push(c);
	}

	_sem.Signal();
}

void Z12Manager::WaitUntilStop()
{
	RUNNER_STATUS status = GetRunStatus();

	if (status == RUNNER_STATUS_RUNNING || 
		status == RUNNER_STATUS_STOPPING)
		_sem.Wait();
}
