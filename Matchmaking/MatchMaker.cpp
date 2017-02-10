#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <math.h>


MatchMaker::RWMutex::RWMutex()
	: mySRWLock(SRWLOCK_INIT)
{
}

MatchMaker::RWMutex::~RWMutex()
{
}

void 
MatchMaker::RWMutex::LockRead()
{
	AcquireSRWLockShared(&mySRWLock);
}

void 
MatchMaker::RWMutex::UnlockRead()
{
	ReleaseSRWLockShared(&mySRWLock);
}

void 
MatchMaker::RWMutex::LockWrite()
{
	AcquireSRWLockExclusive(&mySRWLock);
}

void 
MatchMaker::RWMutex::UnlockWrite()
{
	ReleaseSRWLockExclusive(&mySRWLock);
}


	MatchMaker::MatchMaker()
		: myNumPlayers(0)
	{
	}

	MatchMaker::~MatchMaker()
	{
	}

	MatchMaker&
	MatchMaker::GetInstance()
	{
		static MatchMaker* instance = new MatchMaker(); 
		return *instance; 
	}

	MatchMaker::Player* MatchMaker::FindPlayer(unsigned int	aPlayerId) const
	{
		Player* const * iterPlayers = myPlayers;
		Player* const * endPlayers = myPlayers + myNumPlayers;

		for (; iterPlayers < endPlayers; ++iterPlayers)
		{
			Player* p = *iterPlayers;
			if (p->myPlayerId == aPlayerId)
			{
				return p;
			}
		}

		return nullptr;
	}

	bool
	MatchMaker::AddUpdatePlayer(
		unsigned int	aPlayerId, 
		float			aPreferenceVector[20])
	{
		WriterLock lock(myLock);

		Player* p = FindPlayer(aPlayerId);

		if (p != nullptr)
		{
			p->SetPreferences(aPreferenceVector);
			return true;
		}

		if (myNumPlayers == MAX_NUM_PLAYERS)
			return false;

		myPlayers[myNumPlayers] = new Player(aPlayerId, aPreferenceVector, false);

		++myNumPlayers;

		return true; 
	}

	bool
	MatchMaker::SetPlayerAvailable(
		unsigned int	aPlayerId)
	{
		WriterLock lock(myLock);

		Player* p = FindPlayer(aPlayerId);
		if (p != nullptr)
		{
			p->myIsAvailable = true;
			return true;
		}

		return false; 
	}

	bool
	MatchMaker::SetPlayerUnavailable(
		unsigned int	aPlayerId)
	{
		WriterLock lock(myLock);

		Player* p = FindPlayer(aPlayerId);
		if (p != nullptr)
		{
			p->myIsAvailable = false;
			return true;
		}

		return false; 
	}

	float 
	Dist(
		float	aA[20], 
		float	aB[20])
	{
		//float dist2 = std::inner_product(aA, aA + 20, aB, 0.f, std::plus<float>(), [](float a, float b) 
		//{
		//	float res = a - b;
		//	res *= res;
		//	return res;
		//});
		float dist2 = 0.f;
		for (auto i = 0; i < 20; ++i)
		{
			float d2 = aA[i] - aB[i];
			d2 *= d2;
			dist2 += d2;
		}

		return dist2;
	}

	bool
	MatchMaker::MatchMake(
		unsigned int	aPlayerId, 
		unsigned int	aPlayerIds[20], 
		int&			aOutNumPlayerIds)
	{
		ReaderLock lock(myLock);

		const Player* playerToMatch = FindPlayer(aPlayerId);

		if(!playerToMatch)
			return false; 

		MatchedBinHeap matched;

		Player** endPlayers = myPlayers + myNumPlayers;
		Player** iterPlayers = myPlayers;

		for(; iterPlayers < endPlayers; ++iterPlayers)
		{
			Player * player = *iterPlayers;
			if (!player->myIsAvailable)
				continue;

			float dist = Dist(playerToMatch->myPreferenceVector, player->myPreferenceVector);
			matched.AddItem(player->myPlayerId, dist);
		}

		matched.Export(aPlayerIds, aOutNumPlayerIds);

		return true; 
	}
