#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <thread>
#include <math.h>


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
		matchMaker->RunTask();
	}

	void
	MatchMaker::RunTask()
	{
		bool end = false;
		while(!end)
		{
			DWORD waitResult = WaitForSingleObject(myRunComputeTaskSem, INFINITE);
			if (waitResult == WAIT_OBJECT_0)
			{
				LONG argIndex = InterlockedIncrement(&myComputeTaskArgIndex);
				LONG taskIndex = argIndex - 1;

				ComputeTaskArg& arg = myComputeTaskArgs[taskIndex];

				if (arg.myTaskType == ComputeTaskArg::ETaskType::Compute)
				{
					ComputeDistRange(*arg.myPlayerToMatch, arg.myPlayerToMatchIndex, arg.myBeginIndex, arg.myEndIndex, myComputeResults[taskIndex]);
				}
				else if(arg.myTaskType == ComputeTaskArg::ETaskType::Exit)
				{
					end = true;
				}

				LONG counter = InterlockedDecrement(&myComputeTaskDoneCounter);
				if (counter == 0)
				{
					SetEvent(myComputeTaskDoneEvent);
				}
			}
		}
	}

	void
	MatchMaker::StartTasks(unsigned int taskCount)
	{
		myComputeTaskArgIndex = 0;
		myComputeTaskDoneCounter = taskCount;

		ReleaseSemaphore(myRunComputeTaskSem, taskCount, NULL);
	}



	MatchMaker::MatchMaker()
		: myNumPlayers(0)
		, myComputeTaskArgIndex(0)
		, myComputeTaskDoneCounter(0)
	{
		myNumComputeTasks = std::thread::hardware_concurrency();
		printf("Number of threads : %u\n", myNumComputeTasks);

		myRunComputeTaskSem = CreateSemaphore(NULL, 0, myNumComputeTasks, NULL);
		myComputeTaskDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		myComputeTaskArgs = new ComputeTaskArg[myNumComputeTasks];
		myComputeResults = new MatchedResult[myNumComputeTasks];

		size_t matchedSize = (MAX_NUM_PLAYERS / myNumComputeTasks) + MAX_NUM_PLAYERS;
		
		myComputeTaskThreads = new HANDLE[myNumComputeTasks];
		for (unsigned int i = 0; i < myNumComputeTasks; ++i)
		{
			myComputeResults[i].myResults = new Matched[matchedSize];
			myComputeTaskThreads[i] = (HANDLE)_beginthread(MatchMaker::TaskHandler, 0, (void*)this);
		}		
	}

	MatchMaker::~MatchMaker()
	{
		for (unsigned int i = 0; i < myNumComputeTasks; ++i)
		{
			ComputeTaskArg& arg = myComputeTaskArgs[i];
			arg.myTaskType = ComputeTaskArg::ETaskType::Exit;
		}
		StartTasks(myNumComputeTasks);

		CloseHandle(myRunComputeTaskSem);
		CloseHandle(myComputeTaskDoneEvent);

		delete[] myComputeTaskArgs;
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
			size_t&			aOutPlayerIndex) const
	{
		Player const * iterPlayers = myPlayers;
		Player const * endPlayers = myPlayers + myNumPlayers;

		for (aOutPlayerIndex = 0; iterPlayers < endPlayers; ++iterPlayers, ++aOutPlayerIndex)
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
			printf("************ num players in system %u ********\n", myNumPlayers);


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
	MatchMaker::MatchMake(
		unsigned int	aPlayerId, 
		unsigned int	aPlayerIds[20], 
		int&			aOutNumPlayerIds)
	{
		MutexLock lock(myLock); 

		size_t playerToMatchIndex;
		const Player* playerToMatch = FindPlayer(aPlayerId, playerToMatchIndex);

		if(!playerToMatch)
			return false; 

		MatchedResult& results = myComputeResults[0];
		size_t numPlayers = myNumPlayers;

		if (numPlayers < 20 || numPlayers < myNumComputeTasks)
		{
			ComputeDistRange(*playerToMatch, playerToMatchIndex, 0, numPlayers, results);
			std::sort(results.myResults, results.myResults + results.myCount, MatchComp);
			if (results.myCount > 20)
			{
				results.myCount = 20;
			}
		}
		else
		{
			size_t nbItemPerThread = numPlayers / myNumComputeTasks;
			size_t itemIndex = 0;
			for (size_t i = 0; i < myNumComputeTasks; ++i)
			{
				ComputeTaskArg& arg = myComputeTaskArgs[i];
				arg.Reset(playerToMatch, playerToMatchIndex, itemIndex, (i + 1) == myNumComputeTasks ? numPlayers : itemIndex + nbItemPerThread, ComputeTaskArg::ETaskType::Compute);
				itemIndex += nbItemPerThread;
			}

			StartTasks(myNumComputeTasks);

			DWORD waitResult = WaitForSingleObject(myComputeTaskDoneEvent, INFINITE);

			for (size_t taskIndex = 1; taskIndex < myNumComputeTasks && results.myCount < 20; ++taskIndex)
			{
				MatchedResult& resultsToMerge = myComputeResults[taskIndex];
				while (resultsToMerge.myCount > 0 && results.myCount < 20)
				{
					--resultsToMerge.myCount;
					results.myResults[results.myCount] = resultsToMerge.myResults[resultsToMerge.myCount];
					++results.myCount;
				}
			}

			std::sort(results.myResults, results.myResults + results.myCount, MatchComp);

			if (results.myCount > 20)
			{
				results.myCount = 20;

				for (size_t taskIndex = 1; taskIndex < myNumComputeTasks; ++taskIndex)
				{
					MatchedResult& resultsToMerge = myComputeResults[taskIndex];

					while (resultsToMerge.myCount > 0)
					{
						--resultsToMerge.myCount;
						Matched& itemToMerge = resultsToMerge.myResults[resultsToMerge.myCount];

						int index = -1;
						for (int j = 19; j >= 0; --j)
						{
							if (results.myResults[j].myDist <= itemToMerge.myDist)
								break;

							index = j;
						}

						if (index == -1)
							continue;

						for (int j = 19; j > index; --j)
						{
							results.myResults[j] = results.myResults[j - 1];
						}

						results.myResults[index] = itemToMerge;
					}
				}
			}
		}

		aOutNumPlayerIds = (int)results.myCount;

		for(auto j = 0; j < aOutNumPlayerIds; ++j)
			aPlayerIds[j] = results.myResults[j].myId;

		return true; 
	}


	void
		MatchMaker::ComputeDistRange(
			const Player&	aPlayerToMatch,
			size_t			aPlayerToMatchIndex,
			size_t			aBeginIndex,
			size_t			aEndIndex,
			MatchedResult&	aOutResults)
	{
		aOutResults.myCount = 0;
		const Player* end = myPlayers + aEndIndex;
		const Player* player = myPlayers + aBeginIndex;
		for (size_t iter = aBeginIndex; iter < aEndIndex; ++iter, ++player)
		{
			if (!player->myIsAvailable)
				continue;

			Matched& target = aOutResults.myResults[aOutResults.myCount];
			++aOutResults.myCount;

			target.myId = player->myPlayerId;

			if (player == &aPlayerToMatch)
			{
				target.myDist = 0.f;
			}
			else
			{
				target.myDist = Dist(aPlayerToMatch.myPreferenceVector, player->myPreferenceVector);
			}
		}

		//std::sort(aOutResults.myResults, aOutResults.myResults + aOutResults.myCount, MatchComp);
	}


