#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <math.h>

	MatchMaker::MatchMaker()
		: myNumPlayers(0)
	{
		for (int i = 0; i < 256; ++i)
		{
			for (int j = 0; j < 256; ++j)
			{
				int v = i - j;
				v *= v;
				myDistCache[i][j] = v;
			}
		}
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
		Player const * iterPlayers = myPlayers;
		Player const * endPlayers = myPlayers + myNumPlayers;

		for (; iterPlayers < endPlayers; ++iterPlayers)
		{
			Player const * p = iterPlayers;
			if (p->myPlayerId == aPlayerId)
			{
				return (Player *)p;
			}
		}

		return nullptr;
	}

	bool
	MatchMaker::AddUpdatePlayer(
		unsigned int	aPlayerId, 
		float			aPreferenceVector[20])
	{
		Preference prefVec[20];
		for (int i = 0; i < 20; ++i)
		{
			prefVec[i] = static_cast<Preference>(aPreferenceVector[i] * 255);
		}

		MutexLock lock(myLock); 

		Player* p = FindPlayer(aPlayerId);
		if (p != nullptr)
		{
			p->SetPreferences(prefVec);
			return true;
		}

		if(myNumPlayers == MAX_NUM_PLAYERS)
			return false; 

		Player & newPlayer = myPlayers[myNumPlayers];

		newPlayer.myPlayerId = aPlayerId;
		newPlayer.myIsAvailable = false;
		newPlayer.SetPreferences(prefVec);

		++myNumPlayers; 

		return true; 
	}

	bool
	MatchMaker::SetPlayerAvailable(
		unsigned int	aPlayerId)
	{
		MutexLock lock(myLock); 

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
		MutexLock lock(myLock); 

		Player* p = FindPlayer(aPlayerId);
		if (p != nullptr)
		{
			p->myIsAvailable = false;
			return true;
		}

		return false; 
	}

	int 
	MatchMaker::Dist(
		const Preference	aA[20],
		const Preference	aB[20]) const
	{
		int dist2 = 0;
		for (auto i = 0; i < 20; ++i)
		{
			dist2 += myDistCache[aA[i]][aB[i]];
		}

		return dist2;
	}

	bool 
	MatchMaker::MatchComp(
		Matched*	aA, 
		Matched*	aB)
	{
		return (aA->myDist < aB->myDist);
	}

	bool
	MatchMaker::MatchMake(
		unsigned int	aPlayerId, 
		unsigned int	aPlayerIds[20], 
		int&			aOutNumPlayerIds)
	{
		MutexLock lock(myLock); 

		const Player* playerToMatch = FindPlayer(aPlayerId);

		if(!playerToMatch)
			return false; 

		Matched matchedItems[20];
		Matched* matched[20];

		Matched* pIter = matchedItems;
		Matched* pEnd = matchedItems + 20;
		Matched** p = matched;
		for(; pIter < pEnd; ++pIter, ++p)
		{
			*p = pIter;
		}

		int & matchCount = aOutNumPlayerIds;
		matchCount = 0;

		Player* player = myPlayers;
		Player* endPlayer = myPlayers + myNumPlayers;

		for (; player < endPlayer && matchCount < 20; ++player)
		{
			if (!player->myIsAvailable)
				continue;

			Matched* pItem = matched[matchCount];
			pItem->myId = player->myPlayerId;
			pItem->myDist = Dist(player->myPreferenceVector, playerToMatch->myPreferenceVector);
			++matchCount;

		}

		using std::sort;
		sort(matched, matched + matchCount, MatchComp);

		for(; player < endPlayer; ++player)
		{
			if (!player->myIsAvailable)
				continue;

			int dist = Dist(playerToMatch->myPreferenceVector, player->myPreferenceVector);

			int index = -1; 
			for(int j = 19; j >= 0; --j)
			{
				if(matched[j]->myDist <= dist)
					break; 

				index = j; 
			}

			if(index == -1)
				continue; 

			Matched* newItem = matched[19];
			newItem->myDist = dist;
			newItem->myId = player->myPlayerId;

			for(int j = 19; j > index; --j)
			{
				matched[j] = matched[j - 1];
			}

			matched[index] = newItem;
		}

		for(auto j = 0; j < matchCount; ++j)
			aPlayerIds[j] = matched[j]->myId; 

		return true; 
	}
