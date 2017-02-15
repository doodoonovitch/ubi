#ifndef MUTEX_H
#define MUTEX_H

#include <intrin.h>

#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedCompareExchange)

	class Mutex
	{
	public:

		Mutex()
		{
			InitializeCriticalSection(&myCS);
		}

		~Mutex()
		{
			DeleteCriticalSection(&myCS);
		}

		void
		Lock()
		{
			EnterCriticalSection(&myCS);
		}

		void 
		Unlock()
		{
			LeaveCriticalSection(&myCS);
		}

		CRITICAL_SECTION myCS;
	};

	class MutexLock
	{
	public:

		MutexLock(
			Mutex*	aLock)
			: myLock(aLock)
		{
			myLock->Lock(); 
		}

		MutexLock(
			Mutex&	aLock)
			: myLock(&aLock)
		{
			myLock->Lock(); 
		}

		~MutexLock()
		{
			myLock->Unlock();  
		}

		Mutex*	myLock; 
	};

#endif // MUTEX_H