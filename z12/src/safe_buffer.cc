
#include "safe_buffer.h"

SafeBuffer::SafeBuffer()
{
}

SafeBuffer::~SafeBuffer()
{
}

size_t SafeBuffer::Size() const
{
	return _list.size();
}

bool SafeBuffer::Empty() const
{
	return _list.empty();
}

void SafeBuffer::Push(unsigned char c)
{
	_mutex.Lock();

	_list.push_back(c);

	_mutex.Unlock();
}

unsigned char SafeBuffer::Pop()
{
	unsigned char c;

	_mutex.Lock();

	c = _list.front();

	_list.pop_front();

	_mutex.Unlock();

	return c;
}
