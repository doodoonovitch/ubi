#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <math.h>
#include <malloc.h>
#include <crtdbg.h>

	MatchMaker::MatchMaker()
		: myNumPlayers(0)
		, myPlayers((Player *)_aligned_malloc(sizeof(Player) * MAX_NUM_PLAYERS, 16))
	{
	}

	MatchMaker::~MatchMaker()
	{
		_aligned_free(myPlayers);
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
		MutexLock lock(myLock); 

		Player* p = FindPlayer(aPlayerId);
		if (p != nullptr)
		{
			p->SetPreferences(aPreferenceVector);
			return true;
		}

		if(myNumPlayers == MAX_NUM_PLAYERS)
			return false; 

		Player & newPlayer = myPlayers[myNumPlayers];

		newPlayer.myPlayerId = aPlayerId;
		newPlayer.myIsAvailable = false;
		newPlayer.SetPreferences(aPreferenceVector);

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

	float 
	MatchMaker::Dist(
		const PrefVec aA, 
		const PrefVec aB)
	{
		//float dist2 = std::inner_product(aA, aA + 20, aB, 0.f, std::plus<float>(), [](float a, float b) 
		//{
		//	float res = a - b;
		//	res *= res;
		//	return res;
		//});
		//float dist2 = 0.f;
		//for (auto i = 0; i < 20; ++i)
		//{
		//	float d2 = aA[i] - aB[i];
		//	d2 *= d2;
		//	dist2 += d2;
		//}
		//return dist2;

		float const* pA = aA;
		float const* pB = aB;

		__m128 result;
		__m128 a, b, d;
		a = _mm_load_ps(pA);
		b = _mm_load_ps(pB);
		d = _mm_sub_ps(a, b);
		result = _mm_mul_ps(d, d);
		pA += 4;
		pB += 4;

		for (int i = 1; i < 5; ++i)
		{
			a = _mm_load_ps(pA);
			b = _mm_load_ps(pB);
			d = _mm_sub_ps(a, b);
			__m128 m = _mm_mul_ps(d, d);
			result = _mm_add_ps(result, m);

			pA += 4;
			pB += 4;
		}

		result = _mm_hadd_ps(result, result);
		result = _mm_hadd_ps(result, result);

		//_ASSERT(fabs(result.m128_f32[0] - dist2) <= 0.000001);

		return result.m128_f32[0];
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
