#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <math.h>
#include <thread>


#undef min
#undef max

	MatchMaker::MatchMaker()
		: myNumPlayers(0)
	{
		myDistCompThreadCount = 4;// std::max(2U, std::min(MaxDistCompThreadCount, std::thread::hardware_concurrency()));
		printf("Number of threads for distance computation : %u\n", myDistCompThreadCount);
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
		MutexLock lock(myLock); 

		Player* p = FindPlayer(aPlayerId);
		if (p != nullptr)
		{
			p->SetPreferences(aPreferenceVector);
			return true;
		}

		if(myNumPlayers == MAX_NUM_PLAYERS)
			return false; 

		myPlayers[myNumPlayers] = new Player(aPlayerId, aPreferenceVector, false); 

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

	struct ThreadParam
	{
		float result;
		float* aA;
		float* aB;
		unsigned int count;
	};

	float 
		MatchMaker::Dist(
		float	aA[20], 
		float	aB[20]) const
	{
		HANDLE threads[MaxDistCompThreadCount];
		ThreadParam params[MaxDistCompThreadCount];

		auto d = 20 / myDistCompThreadCount;
		auto r = 20 % myDistCompThreadCount;


		float* pA = aA;
		float* pB = aB;
		
		ThreadParam * param = params;
		HANDLE * threadEnd = threads + myDistCompThreadCount;
		for (HANDLE * thread = threads; thread < threadEnd; ++thread, ++param)
		{
			unsigned int count = d;
			if (r > 0)
			{
				++count;
				--r;
			}

			param->aA = pA;
			param->aB = pB;
			param->count = count;

			*thread = (HANDLE)_beginthread([](void * arg)
			{
				ThreadParam * param = reinterpret_cast<ThreadParam*>(arg);

				param->result = 0.f;
				float* end = param->aA + param->count;
				for (; param->aA < end; ++param->aA, ++param->aB)
				{
					float d2 = *param->aA - *param->aB;
					d2 *= d2;
					param->result += d2;
				}
			}, 0, (void*)param);

			pA += count;
			pB += count;
		}

		WaitForMultipleObjects(myDistCompThreadCount, threads, TRUE, INFINITE);
		
		float dist2 = 0.f;
		ThreadParam* end = params + myDistCompThreadCount;
		ThreadParam* p = params;
		for (; p < end; ++p)
		{
			dist2 += p->result;
		}

		////float dist2 = std::inner_product(aA, aA + 20, aB, 0.f, std::plus<float>(), [](float a, float b) 
		////{
		////	float res = a - b;
		////	res *= res;
		////	return res;
		////});
		//float dist2 = 0.f;
		//for (auto i = 0; i < 20; ++i)
		//{
		//	float d2 = aA[i] - aB[i];
		//	d2 *= d2;
		//	dist2 += d2;
		//}

		return dist2;
	}

	class Matched
	{
	public:

		Matched()
			: myDist(-1.f)
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

		Player** iterPlayers = myPlayers;
		Player** endPlayers = myPlayers + myNumPlayers;

		for (; iterPlayers < endPlayers && matchCount < 20; ++iterPlayers)
		{
			Player * player = *iterPlayers;
			if (!player->myIsAvailable)
				continue;

			Matched* pItem = matched[matchCount];
			pItem->myId = player->myPlayerId;
			pItem->myDist = Dist(player->myPreferenceVector, playerToMatch->myPreferenceVector);
			++matchCount;

		}

		using std::sort;
		sort(matched, matched + matchCount, MatchComp);

		for(; iterPlayers < endPlayers; ++iterPlayers)
		{
			Player * player = *iterPlayers;
			if (!player->myIsAvailable)
				continue;

			float dist = Dist(playerToMatch->myPreferenceVector, player->myPreferenceVector);

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
