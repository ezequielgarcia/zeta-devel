#ifndef _RUNNER_H_
#define _RUNNER_H_

#include "thread.h"
#include "semaphore.h"

enum RUNNER_STATUS {

	RUNNER_STATUS_STOPPED 	= 0,
	RUNNER_STATUS_STOPPING 	= 1,
	RUNNER_STATUS_RUNNING 	= 2,
	RUNNER_STATUS_DESTROYING = 3,
};
                             
class Runner
{

public:
	Runner();
    virtual ~Runner();
	
	void Start();
	void Stop();
	
	bool IsStopping() const;

	RUNNER_STATUS GetRunStatus() const;

protected:

	static void* 	_ThreadEntry(void* arg);

    virtual void 	_Run() = 0;

private:

	RUNNER_STATUS	_enmStatus;

    bool   			_blnIsDying;

	bool			_blnStopping;

    Thread			_clsThread;

	Semaphore		_clsSemStart;
};

//
// Ejemplo de utilizacion de runner. Se debe implementar la rutina _Run(). Para iniciar se ejecuta Start().
//
/*
class Skeleton : public Runner {

private:
	

protected: 

	/// Rutina principal
	void _Run();

public:

	~Skeleton();
	
    Skeleton();

}; 
*/

#endif
