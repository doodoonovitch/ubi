#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <math.h>

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
		for (auto i = 0; i < myNumPlayers; i++)
		{
			Player* p = myPlayers[i];
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
		MutexLock lock(myLock); 

		Player* p = FindPlayer(aPlayerId);
		if (p != nullptr)
		{
			p->SetPreferences(aPreferenceVector);
			return true;
		}

		//for(unsigned int i = 0; i < myNumPlayers; i++)
		//{
		//	if(myPlayers[i]->myPlayerId == aPlayerId)
		//	{
		//		Player* t = new Player(); 
		//		t->myPlayerId = myPlayers[i]->myPlayerId; 
		//		t->myIsAvailable = myPlayers[i]->myIsAvailable; 

		//		for(int j = 0; j < 20; j++)
		//			t->myPreferenceVector[j] = aPreferenceVector[j]; 

		//		delete myPlayers[i]; 
		//		myPlayers[i] = t; 

		//		return true; 
		//	}
		//}

		if(myNumPlayers == MAX_NUM_PLAYERS)
			return false; 

		myPlayers[myNumPlayers] = new Player(aPlayerId, aPreferenceVector, false); 
		//myPlayers[myNumPlayers] = new Player();
		//myPlayers[myNumPlayers]->myPlayerId = aPlayerId; 
		//for(int i = 0; i < 20; i++)
		//	myPlayers[myNumPlayers]->myPreferenceVector[i] = aPreferenceVector[i]; 
		//myPlayers[myNumPlayers]->myIsAvailable = false; 

		myNumPlayers++; 

		if(myNumPlayers % 100 == 0)
			printf("num players in system %u\n", myNumPlayers); 

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


		//for(unsigned int i = 0; i < myNumPlayers; i++)
		//{
		//	if(myPlayers[i]->myPlayerId == aPlayerId)
		//	{
		//		Player* t = new Player(); 
		//		t->myPlayerId = myPlayers[i]->myPlayerId; 
		//		t->myIsAvailable = true; 
		//		for(int j = 0; j < 20; j++)
		//			t->myPreferenceVector[j] = myPlayers[i]->myPreferenceVector[j]; 

		//		delete myPlayers[i]; 
		//		myPlayers[i] = t; 

		//		return true; 
		//	}
		//}

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

		//for(unsigned int i = 0; i < myNumPlayers; i++)
		//{
		//	if(myPlayers[i]->myPlayerId == aPlayerId)
		//	{
		//		Player* t = new Player(); 
		//		t->myPlayerId = myPlayers[i]->myPlayerId; 
		//		t->myIsAvailable = false; 
		//		for(int j = 0; j < 20; j++)
		//			t->myPreferenceVector[j] = myPlayers[i]->myPreferenceVector[j]; 

		//		delete myPlayers[i]; 
		//		myPlayers[i] = t; 

		//		return true; 
		//	}
		//}

		return false; 
	}

	float 
	Dist(
		float	aA[20], 
		float	aB[20])
	{
		//float dist = 0.0f; 
		//for(int i = 0; i < 20; i++)
		//	dist += pow((aA[i] - aB[i]), 2.0f); 

		////return sqrt(dist); 
		//return dist;

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

	class Matched
	{
	public:

		Matched()
			: myDist(-1.f)
			, myId(-1)
		{ }

		float			myDist; 
		unsigned int	myId; 
	};

	static bool 
	MatchComp(
		Matched*	aA, 
		Matched*	aB)
	{
		return (aA->myDist < aB->myDist);
		//if(aA->myDist < aB->myDist)
		//	return 1; 

		//return 0; 
	}

	bool
	MatchMaker::MatchMake(
		unsigned int	aPlayerId, 
		unsigned int	aPlayerIds[20], 
		int&			aOutNumPlayerIds)
	{
		MutexLock lock(myLock); 

		//Player* playerToMatch = NULL; 
		//for(unsigned int i = 0; i < myNumPlayers; i++)
		//{
		//	if(myPlayers[i]->myPlayerId == aPlayerId)
		//	{
		//		playerToMatch					= new Player();
		//		playerToMatch->myPlayerId		= myPlayers[i]->myPlayerId;
		//		playerToMatch->myIsAvailable	= myPlayers[i]->myIsAvailable; 
		//		for(int j = 0; j < 20; j++)
		//			playerToMatch->myPreferenceVector[j] = myPlayers[i]->myPreferenceVector[j]; 

		//		break; 
		//	}
		//}

		const Player* playerToMatch = FindPlayer(aPlayerId);

		if(!playerToMatch)
			return false; 

		//Matched* matched = new Matched*[20]; 
		Matched matchedItems[20];
		Matched* matched[20];
		for(unsigned int i = 0; i < 20; i++)
		{
			matched[i] = &matchedItems[i];
			//matched[i]			= new Matched(); 
			//matched[i]->myDist	= -1.0f; 
			//matched[i]->myId	= -1; 
		}

		int matchCount = 0; 
		int playerIndex;
		for (playerIndex = 0; playerIndex < myNumPlayers && playerIndex < 20; playerIndex++)
		{
				matched[matchCount]->myId = myPlayers[playerIndex]->myPlayerId;
				matched[matchCount]->myDist = Dist(myPlayers[playerIndex]->myPreferenceVector, playerToMatch->myPreferenceVector);
				matchCount++;

		}

		using std::sort;
		sort(matched, matched + matchCount, MatchComp);

		for(/*auto playerIndex = 0*/; playerIndex < myNumPlayers; playerIndex++)
		{
			//if(matchCount < 20)
			//{
			//	matched[matchCount]->myId	= myPlayers[playerIndex]->myPlayerId; 
			//	matched[matchCount]->myDist	= Dist(myPlayers[playerIndex]->myPreferenceVector, playerToMatch->myPreferenceVector);
			//	matchCount++; 

			//	using std::sort; 
			//	sort(matched, matched + matchCount, MatchComp);

			//	continue; 
			//}

			if (!myPlayers[playerIndex]->myIsAvailable)
				continue;

			float dist = Dist(playerToMatch->myPreferenceVector, myPlayers[playerIndex]->myPreferenceVector);

			int index = -1; 
			for(int j = 19; j >= 0; j--)
			{
				if(matched[j]->myDist < dist)
					break; 

				index = j; 
			}

			if(index == -1)
				continue; 

			for(int j = 19; j > index; j--)
			{
				matched[j]->myDist	= matched[j - 1]->myDist; 
				matched[j]->myId	= matched[j - 1]->myId; 
			}

			matched[index]->myDist	= dist;
			matched[index]->myId	= myPlayers[playerIndex]->myPlayerId;

			for(int j = 0; j < 20; j++)
				aPlayerIds[j] = matched[j]->myId; 
		}

		aOutNumPlayerIds = matchCount; 
		for(auto j = 0; j < matchCount; j++)
			aPlayerIds[j] = matched[j]->myId; 

		//for(unsigned int i = 0; i < 20; i++)
		//	delete matched[i]; 
		//delete [] matched; 

		//delete playerToMatch; 

		return true; 
	}
