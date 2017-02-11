#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <math.h>


	void			
	MatchMaker::TaskHandler(
		void*	aData)
	{
		MatchMakeTask * task = (MatchMakeTask *)aData;
		task->Run();
	}


#undef min
#undef max

	MatchMaker::MatchMaker()
		: myNumPlayers(0)
		, myThreadCount(0)
	{
		myThreadCount = std::max(2U, std::min(MaxThreadCount, std::thread::hardware_concurrency()));
		printf("Number of threads : %u\n", myThreadCount);

		myMatched = new Matched*[myThreadCount * 20];
		myMatchedBinHeap = new MatchedBinHeap[myThreadCount];
		myTasks = new MatchMakeTask[myThreadCount];


	}

	MatchMaker::~MatchMaker()
	{
		delete[] myTasks;
		delete[] myMatched;
		delete[] myMatchedBinHeap;
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
		MutexLock lock(myLock); 

		const Player* playerToMatch = FindPlayer(aPlayerId);

		if(!playerToMatch)
			return false; 

		HANDLE taskHandles[MaxThreadCount];

		Player** iterPlayers = myPlayers;
		Player** endPlayers = myPlayers + myNumPlayers;
		int nbPlayerPerThread = myNumPlayers / myThreadCount;
		
		unsigned int n = myThreadCount - 1;
		unsigned int taskIndex = 0;
		for (; taskIndex < myThreadCount; ++taskIndex)
		{
			MatchMakeTask & task = myTasks[taskIndex];
			MatchedBinHeap & matched = myMatchedBinHeap[taskIndex];
			matched.Reset();

			if (taskIndex == n)
			{
				task.Reset(playerToMatch, &matched, iterPlayers, endPlayers);
			}
			else
			{
				Player** iterPlayersNext = iterPlayers + nbPlayerPerThread;
				task.Reset(playerToMatch, &matched, iterPlayers, iterPlayersNext);
				iterPlayers = iterPlayersNext;
			}
			taskHandles[taskIndex] = (HANDLE)_beginthread(TaskHandler, 0, (void*)&task);
		}

		WaitForMultipleObjects(myThreadCount, taskHandles, TRUE, INFINITE);

		float maxDist;

		unsigned int matchedCount = 0;

		for (unsigned int i = 0; i < myThreadCount; ++i)
		{
			MatchedBinHeap& bh = myMatchedBinHeap[i];

			if (matchedCount == 0)
			{
				matchedCount = bh.GetSize();
				if (matchedCount > 0)
				{
					memcpy(myMatched, bh.GetArray(), bh.GetSize() * sizeof(Matched *));
					maxDist = myMatched[0]->myDist;
				}
			}
			else
			{
				Matched ** trg = myMatched + matchedCount;
				Matched *const* src = bh.GetArray();
				Matched *const* end = src + bh.GetSize();
				for (; src < end; ++src)
				{
					if ((*src)->myDist < maxDist)
					{
						*trg = *src;
						++trg;
						++matchedCount;
					}
				}
			}
		}

		aOutNumPlayerIds = std::min(20, static_cast<int>(matchedCount));
		if (matchedCount > 0)
		{
			std::sort(myMatched, myMatched + matchedCount, MatchComp);
			for (int i = 0; i < aOutNumPlayerIds; ++i)
			{
				aPlayerIds[i] = myMatched[i]->myId;
			}
		}


		return true; 
	}


	MatchMaker::MatchMakeTask::MatchMakeTask(
	)
	{

	}

	MatchMaker::MatchMakeTask::~MatchMakeTask()
	{
	}

	void
	MatchMaker::MatchMakeTask::Reset(
		const Player*		aPlayerToMatch,
		MatchedBinHeap*		aMatched,
		Player**			aPlayersBeginIter,
		Player**			aPlayersEndIter
	)
	{
		myPlayerToMatch = aPlayerToMatch;
		myMatched = aMatched;
		myPlayersBeginIter = aPlayersBeginIter;
		myPlayersEndIter = aPlayersEndIter;
	}

	void MatchMaker::MatchMakeTask::Run()
	{
		for (Player** iterPlayers = myPlayersBeginIter; iterPlayers < myPlayersEndIter; ++iterPlayers)
		{
			Player * player = *iterPlayers;
			if (!player->myIsAvailable)
				continue;

			float dist = Dist(myPlayerToMatch->myPreferenceVector, player->myPreferenceVector);
			myMatched->AddItem(player->myPlayerId, dist);
		}
	}
