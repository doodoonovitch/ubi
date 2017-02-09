#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <math.h>


	void			
	MatchMaker::TaskHandler(
		MatchMaker *	matchMaker, 
		unsigned int	index)
	{
		matchMaker->RunTasks(index);
	}

	void				
	MatchMaker::RunTasks(
		unsigned int	index)
	{
		for (;;)
		{
			DWORD waitResult = WaitForSingleObject(myRunTaskSemaphore, INFINITE);
			if (waitResult == WAIT_OBJECT_0)
			{
				MatchMakeTask & task = myTaskStorage[index];

				task.Run();
			}
			else
			{
				break;
			}
		}
	}

#undef min
#undef max

	MatchMaker::MatchMaker()
		: myNumPlayers(0)
		, myThreadCount(0)
		, myThreads(nullptr)
		, myTaskStorage(nullptr)
		, myRunTaskSemaphore(CreateSemaphore(NULL, 0, MaxThreadCount, NULL))
	{
		myThreadCount = std::max(2U, std::min(MaxThreadCount, std::thread::hardware_concurrency()));
		printf("Number of threads : %u\n", myThreadCount);
		myThreads = new std::thread[myThreadCount];
		myTaskStorage = new MatchMakeTask[myThreadCount];
		myWaitTaskEvents = new HANDLE[myThreadCount];

		for (unsigned int i = 0; i < myThreadCount; ++i)
		{
			myWaitTaskEvents[i] = myTaskStorage[i].GetSynchEvent();
			myThreads[i] = std::thread(MatchMaker::TaskHandler, this, i);
		}
	}

	MatchMaker::~MatchMaker()
	{
		CloseHandle(myRunTaskSemaphore);

		delete[] myThreads;
		delete[] myTaskStorage;
		delete[] myWaitTaskEvents;
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

		MatchedBinHeap matched[MaxThreadCount];
		MatchedBinHeap & matched0 = *matched;

		Player** iterPlayers = myPlayers;
		Player** endPlayers = myPlayers + myNumPlayers;
		int nbPlayerPerThread = myNumPlayers / myThreadCount;
		
		unsigned int n = myThreadCount - 1;
		unsigned int taskIndex = 0;
		for (taskIndex; taskIndex < n; ++taskIndex)
		{
			MatchMakeTask & task = myTaskStorage[taskIndex];
			Player** iterPlayersNext = iterPlayers + nbPlayerPerThread;
			task.Reset(playerToMatch, &matched[taskIndex], iterPlayers, iterPlayersNext);
			iterPlayers = iterPlayersNext;
		}

		MatchMakeTask & task = myTaskStorage[taskIndex];
		task.Reset(playerToMatch, &matched[taskIndex], iterPlayers, endPlayers);

		ReleaseSemaphore(myRunTaskSemaphore, myThreadCount, NULL);

		WaitForMultipleObjects(myThreadCount, myWaitTaskEvents, TRUE, INFINITE);

		for (unsigned int i = 1; i < myThreadCount; ++i)
		{
			matched[i].ForEach([&matched0](const Matched* item)
			{
				matched0.AddItem(item->myId, item->myDist);
			});
		}

		matched->Export(aPlayerIds, aOutNumPlayerIds);

		return true; 
	}


	MatchMaker::MatchMakeTask::MatchMakeTask(
	)
		: myPlayerToMatch(nullptr)
		, myMatched(nullptr)
		, myPlayersBeginIter(nullptr)
		, myPlayersEndIter(nullptr)
		, mySyncEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
	{

	}

	MatchMaker::MatchMakeTask::~MatchMakeTask()
	{
		CloseHandle(mySyncEvent);
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

		//ResetEvent(mySyncEvent);
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

		SetEvent(mySyncEvent);
	}
