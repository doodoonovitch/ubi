#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <thread>
#include <math.h>
#include <crtdbg.h>


	void 
	MatchMaker::ComputeTaskArg::Reset(
		const Player*	aPlayerToMatch,
		size_t			aPlayerToMatchIndex,
		size_t			aBeginIndex,
		size_t			aEndIndex,
		ETaskType		aTaskType)
	{
		myPlayerToMatch = aPlayerToMatch;
		myPlayerToMatchIndex = aPlayerToMatchIndex;
		myBeginIndex = aBeginIndex;
		myEndIndex = aEndIndex;
		myTaskType = aTaskType;
	}

	void
		MatchMaker::TaskHandler(
			void*			arg)
	{
		MatchMaker * matchMaker = (MatchMaker*)arg;
		matchMaker->RunTasks();
	}

	void
		MatchMaker::RunTasks()
	{
		for (;;)
		{
			DWORD waitResult = WaitForSingleObject(myRunComputeTaskSem, INFINITE);
			if (waitResult == WAIT_OBJECT_0)
			{
				LONG argIndex = InterlockedIncrement(&myComputeTaskArgIndex);

				const ComputeTaskArg& arg = myComputeTaskArgs[argIndex - 1];

				if (arg.myTaskType == ComputeTaskArg::ETaskType::Reset)
				{
					ResetDistRange(arg.myPlayerToMatchIndex, arg.myBeginIndex, arg.myEndIndex);
				}
				else
				{
					ComputeDistRange(*arg.myPlayerToMatch, arg.myPlayerToMatchIndex, arg.myBeginIndex, arg.myEndIndex);
				}

				LONG counter = InterlockedIncrement(&myComputeTaskDoneCounter);
				if (counter == myNumComputeTasks)
				{
					SetEvent(myComputeTaskDoneEvent);
				}
			}
		}
	}


	MatchMaker::MatchMaker()
		: myNumPlayers(0)
		, myComputeTaskArgIndex(0)
		, myComputeTaskDoneCounter(0)
		
	{
		myDists1 = new float[myMaxNumDistsPart1];
		myDists2 = new float[myMaxNumDistsPart2];
		myDists3 = new float[myMaxNumDistsPart3];
		myDists4 = new float[myMaxNumDistsPart4];
		for (size_t i = 0; i < myMaxNumDistsPart1; ++i)
		{
			myDists1[i] = -1.f;
		}
		for (size_t i = myMaxNumDistsPart1; i < myMaxNumDistsPart2; ++i)
		{
			myDists2[i] = -1.f;
		}

		myNumComputeTasks = std::thread::hardware_concurrency();
		printf("Number of threads : %u\n", myNumComputeTasks);
		
		myRunComputeTaskSem = CreateSemaphore(NULL, 0, myNumComputeTasks, NULL);
		myComputeTaskDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		
		myComputeTaskArgs = new ComputeTaskArg[myNumComputeTasks];

		myComputeTaskThreads = new HANDLE[myNumComputeTasks];
		for (unsigned int i = 0; i < myNumComputeTasks; ++i)
		{
			myComputeTaskThreads[i] = (HANDLE)_beginthread(MatchMaker::TaskHandler, 0, (void*)this);
		}

	}

	MatchMaker::~MatchMaker()
	{
		delete[] myDists1;
		delete[] myDists2;
	}

	MatchMaker&
	MatchMaker::GetInstance()
	{
		static MatchMaker* instance = new MatchMaker(); 
		return *instance; 
	}

	MatchMaker::Player* 
	MatchMaker::FindPlayer(
		unsigned int	aPlayerId,
		size_t&			aOutIndex) const
	{
		Player const * iterPlayers = myPlayers;
		Player const * endPlayers = myPlayers + myNumPlayers;

		for (aOutIndex = 0; iterPlayers < endPlayers; ++iterPlayers, ++aOutIndex)
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

		size_t playerIndex;
		Player* p = FindPlayer(aPlayerId, playerIndex);

		if (p != nullptr)
		{
			p->SetPreferences(aPreferenceVector);
			//ResetOrComputeDist(*p, playerIndex, myNumPlayers, ComputeTaskArg::ETaskType::Reset);
			return true;
		}

		if(myNumPlayers == MAX_NUM_PLAYERS)
			return false; 

		Player & newPlayer = myPlayers[myNumPlayers];

		newPlayer.myPlayerId = aPlayerId;
		newPlayer.myIsAvailable = false;
		newPlayer.SetPreferences(aPreferenceVector);

		++myNumPlayers; 

		if (myNumPlayers % 100 == 0)
			printf("num players in system %zu\n", myNumPlayers);


		//ResetOrComputeDist(newPlayer, playerIndex, myNumPlayers, ComputeTaskArg::ETaskType::Compute);

		return true; 
	}

	bool
	MatchMaker::SetPlayerAvailable(
		unsigned int	aPlayerId)
	{
		MutexLock lock(myLock); 

		size_t playerIndex;
		Player* p = FindPlayer(aPlayerId, playerIndex);
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

		size_t playerIndex;
		Player* p = FindPlayer(aPlayerId, playerIndex);
		if (p != nullptr)
		{
			p->myIsAvailable = false;
			return true;
		}

		return false; 
	}

	float 
	Dist(
		const float	aA[20], 
		const float	aB[20])
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
		MatchMaker::MatchComp(
			Matched const&	aA,
			Matched const&	aB)
	{
		return (aA.myDist < aB.myDist);
	}

	bool
	MatchMaker::MatchMake(
		unsigned int	aPlayerId, 
		unsigned int	aPlayerIds[20], 
		int&			aOutNumPlayerIds)
	{
		Matched matchedItems[20];
		Matched* matched = matchedItems;

		int & matchCount = aOutNumPlayerIds;
		matchCount = 0;

		{
			MutexLock lock(myLock);

			size_t playerToMatchIndex;
			const Player* playerToMatch = FindPlayer(aPlayerId, playerToMatchIndex);

			if (!playerToMatch)
				return false;

			Player* player = myPlayers;
			Player* endPlayer = myPlayers + myNumPlayers;

			size_t playerIndex = 0;
			for (; player < endPlayer && matchCount < 20; ++player, ++playerIndex)
			{
				if (!player->myIsAvailable)
					continue;

				Matched& item = matched[matchCount];
				item.myId = player->myPlayerId;
				//item.myDist = Dist(player->myPreferenceVector, playerToMatch->myPreferenceVector);
				float& dist = GetDist(playerToMatchIndex, playerIndex);
				if (dist < 0.f)
				{
					//ResetOrComputeDist(*playerToMatch, playerToMatchIndex, myNumPlayers, ComputeTaskArg::ETaskType::Compute);
					dist = Dist(player->myPreferenceVector, playerToMatch->myPreferenceVector);
				}
				item.myDist = dist;
				++matchCount;

			}

			using std::sort;
			sort(matched, matched + matchCount, MatchComp);

			for (; player < endPlayer; ++player, ++playerIndex)
			{
				if (!player->myIsAvailable)
					continue;

				//float dist = Dist(playerToMatch->myPreferenceVector, player->myPreferenceVector);
				float& dist = GetDist(playerToMatchIndex, playerIndex);
				if (dist < 0.f)
				{
					dist = Dist(player->myPreferenceVector, playerToMatch->myPreferenceVector);
					//ResetOrComputeDist(*playerToMatch, playerToMatchIndex, myNumPlayers, ComputeTaskArg::ETaskType::Compute);
				}

				int index = -1;
				for (int j = 19; j >= 0; --j)
				{
					if (matched[j].myDist <= dist)
						break;

					index = j;
				}

				if (index == -1)
					continue;

				Matched& newItem = matched[19];
				newItem.myDist = dist;
				newItem.myId = player->myPlayerId;

				for (int j = 19; j > index; --j)
				{
					matched[j] = matched[j - 1];
				}

				matched[index] = newItem;
			}
		}

		for(auto j = 0; j < matchCount; ++j)
			aPlayerIds[j] = matched[j].myId; 

		return true; 
	}
	
	void
		MatchMaker::ComputeDistRange(
			const Player&	aPlayerToMatch,
			size_t			aPlayerToMatchIndex,
			size_t			aBeginIndex,
			size_t			aEndIndex)
	{
		for (; aBeginIndex < aEndIndex; ++aBeginIndex)
		{
			float& dist = GetDist(aPlayerToMatchIndex, aBeginIndex);

			if (aBeginIndex == aPlayerToMatchIndex)
			{
				dist = 0.f;
			}
			else
			{
				if (dist < 0.f)
				{
					const Player& playerB = myPlayers[aBeginIndex];
					dist = Dist(aPlayerToMatch.myPreferenceVector, playerB.myPreferenceVector);
				}
			}
		}
	}

	void
		MatchMaker::ResetOrComputeDist(
			const Player&	aPlayerToMatch,
			size_t			aPlayerToMatchIndex,
			size_t			aNumPlayers,
			ComputeTaskArg::ETaskType aTaskType)
	{
		size_t nbItemPerThread = aNumPlayers / myNumComputeTasks;
		size_t itemIndex = 0;
		for (size_t i = 0; i < myNumComputeTasks; ++i)
		{
			ComputeTaskArg& arg = myComputeTaskArgs[i];
			arg.Reset(&aPlayerToMatch, aPlayerToMatchIndex, itemIndex, (i + 1) == myNumComputeTasks ? aNumPlayers : itemIndex + nbItemPerThread, aTaskType);
			itemIndex += nbItemPerThread;
		}

		InterlockedXor(&myComputeTaskArgIndex, myComputeTaskArgIndex);
		InterlockedXor(&myComputeTaskDoneCounter, myComputeTaskDoneCounter);

		ReleaseSemaphore(myRunComputeTaskSem, myNumComputeTasks, NULL);

		DWORD waitResult = WaitForSingleObject(myComputeTaskDoneEvent, INFINITE);
	}

	void
		MatchMaker::ResetDistRange(
			size_t			aPlayerToMatchIndex,
			size_t			aBeginIndex,
			size_t			aEndIndex)
	{
		for (; aBeginIndex < aEndIndex; ++aBeginIndex)
		{
			if (aBeginIndex != aPlayerToMatchIndex)
			{
				float& dist = GetDist(aPlayerToMatchIndex, aBeginIndex);
				dist = -1.f;
			}
		}
	}
