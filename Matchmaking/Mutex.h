#ifndef MUTEX_H
#define MUTEX_H

#include <intrin.h>

#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedCompareExchange)

	class Mutex
	{
	public:

		Mutex()
			//: mySpinLock(0)
			: mySysMutex(::CreateMutex(NULL, FALSE, NULL))
		{
		}

		~Mutex()
		{
			CloseHandle(mySysMutex);
			mySysMutex = NULL;
		}

		void
		Lock()
		{
			//while(_InterlockedCompareExchange(&mySpinLock, 1, 0)); 
			WaitForSingleObject(mySysMutex, INFINITE);
		}

		void 
		Unlock()
		{
			//_InterlockedExchange(&mySpinLock, 0); 
			ReleaseMutex(mySysMutex);
		}

		//volatile LONG	mySpinLock; 
		HANDLE mySysMutex;
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