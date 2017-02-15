#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#define MAX_NUM_PLAYERS (1000000)

#include "Mutex.h"

class MatchMaker
{
public:

	// keep these methods
	static MatchMaker&	GetInstance(); 

	bool				AddUpdatePlayer(
							unsigned int	aPlayerId, 
							float			aPreferenceVector[20]); 

	bool				SetPlayerAvailable(
							unsigned int	aPlayerId); 

	bool				SetPlayerUnavailable(
							unsigned int	aPlayerId); 

	bool				MatchMake(
							unsigned int	aPlayerId, 
							unsigned int	aPlayerIds[20], 
							int&			aOutNumPlayerIds); 

private: 

	// I don't care if you change anything below this 

	class Player 
	{
	public:

		//Player(
		//	unsigned int	aPlayerId,
		//	float			aPreferenceVector[20],
		//	bool			aIsAvailable
		//)
		//	: myPlayerId(aPlayerId)
		//	, myPreferenceVector(new float[20])
		//	, myIsAvailable(aIsAvailable)
		//{
		//	SetPreferences(aPreferenceVector);
		//}

		Player()
		{ }

		~Player()
		{
			//delete [] myPreferenceVector; 
		}

		void			SetPreferences(
							float aPreferenceVector[20])
		{
			memcpy(myPreferenceVector, aPreferenceVector, sizeof(float[20]));
		}

		float			myPreferenceVector[20];
		unsigned int	myPlayerId;
		bool			myIsAvailable; 
	};


	Player*				FindPlayer(
							unsigned int	aPlayerId) const;

	class RWMutex
	{
	public:

		RWMutex();
		~RWMutex();

		void LockRead();
		void UnlockRead();
		void LockWrite();
		void UnlockWrite();

	private:

		SRWLOCK				mySRWLock;
	};

	class ReaderLock
	{
	public:

		ReaderLock(
			RWMutex*	aLock)
			: myLock(aLock)
		{
			myLock->LockRead();
		}

		ReaderLock(
			RWMutex&	aLock)
			: myLock(&aLock)
		{
			myLock->LockRead();
		}

		~ReaderLock()
		{
			myLock->UnlockRead();
		}

		RWMutex*	myLock;
	};


	class WriterLock
	{
	public:

		WriterLock(
			RWMutex*	aLock)
			: myLock(aLock)
		{
			myLock->LockWrite();
		}

		WriterLock(
			RWMutex&	aLock)
			: myLock(&aLock)
		{
			myLock->LockWrite();
		}

		~WriterLock()
		{
			myLock->UnlockWrite();
		}

		RWMutex*	myLock;
	};


	RWMutex				myLock;
	Mutex				myLockAdd;
	Player				myPlayers[MAX_NUM_PLAYERS]; 
	int					myNumPlayers;

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H