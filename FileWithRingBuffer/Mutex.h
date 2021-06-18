#pragma once

#include <windows.h>

namespace Mutex {

class Mutex
{
private:
	CRITICAL_SECTION p_cs;
public:
	Mutex()
	{
		InitializeCriticalSection(&p_cs);
	}

	~Mutex()
	{
		DeleteCriticalSection(&p_cs);
	}

	void lock()
	{
		EnterCriticalSection(&p_cs);
	}

	void unlock()
	{
		LeaveCriticalSection(&p_cs);
	}
};

} // namespace Mutex
