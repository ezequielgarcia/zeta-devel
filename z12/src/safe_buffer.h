
#ifndef _SAFE_VECTOR_H_
#define _SAFE_VECTOR_H_

#include "mutex.h"
#include <list>

using namespace std;

class SafeBuffer
{

public:
	
	SafeBuffer();
    ~SafeBuffer();

	void Push(unsigned char c);

	unsigned char Pop();

	size_t Size() const;

	bool Empty() const;

private:
    Mutex _mutex;
	list<unsigned char> _list;
   
};

#endif
