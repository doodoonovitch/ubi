#include "stdafx.h"

#include "MatchMaker.h"

#include <algorithm>
#include <numeric>
#include <functional>
#include <thread>
#include <math.h>


	void
	MatchMaker::ComputeTaskArg::InitComputeTask(
		const Player*	aPlayerToMatch,
		size_t			aPlayerToMatchIndex,
		size_t			aBeginIndex,
		size_t			aEndIndex)
	{
		myUnion.myComputeTask.myPlayerToMatch = aPlayerToMatch;
		myUnion.myComputeTask.myPlayerToMatchIndex = aPlayerToMatchIndex;
		myBeginIndex = aBeginIndex;
		myEndIndex = aEndIndex;
		myTaskType = ETaskType::Compute;
	}

	void
	MatchMaker::ComputeTaskArg::InitFindTask(
		unsigned int	aPlayerId,
		size_t			aBeginIndex,
		size_t			aEndIndex)
	{
		myUnion.myFindTask.myPlayerId = aPlayerId;
		myBeginIndex = aBeginIndex;
		myEndIndex = aEndIndex;
		myTaskType = ETaskType::Find;
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

				switch (arg.myTaskType)
				{
				case ComputeTaskArg::ETaskType::Compute:
					ComputeDistRange(*arg.myUnion.myComputeTask.myPlayerToMatch, arg.myUnion.myComputeTask.myPlayerToMatchIndex, arg.myBeginIndex, arg.myEndIndex, myComputeResults[taskIndex]);
					break;

				case ComputeTaskArg::ETaskType::Find:
					{
						size_t index;
						Player* player = FindPlayerRange(arg.myUnion.myFindTask.myPlayerId, arg.myBeginIndex, arg.myEndIndex, index);
						if (player != nullptr)
						{
							myFindTaskPlayer = player;
							myFindTaskPlayerIndex = index;
							myFindTaskPlayerFound = true;
						}
					}
					break;

				case ComputeTaskArg::ETaskType::Exit:
					end = true;
					break;
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
		, myAvailablePlayers(0)
		, myComputeTaskArgIndex(0)
		, myComputeTaskDoneCounter(0)
		, myFindTaskPlayer(nullptr)
		, myFindTaskPlayerIndex(MAX_NUM_PLAYERS)
		, myFindTaskPlayerFound(false)
		, myCacheResultPlayer(nullptr)
		, myCacheHitCount(0)
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
			size_t&			aOutPlayerIndex) 
	{
		if (myFindTaskPlayer != nullptr && myFindTaskPlayer->myPlayerId == aPlayerId)
		{
			aOutPlayerIndex = myFindTaskPlayerIndex;
			return myFindTaskPlayer;
		}

		size_t numPlayers = myNumPlayers;

		if (numPlayers < 20 || numPlayers < myNumComputeTasks)
		{
			Player* player = FindPlayerRange(aPlayerId, 0, numPlayers, aOutPlayerIndex);
			if (player != nullptr)
			{
				myFindTaskPlayer = player;
				myFindTaskPlayerIndex = aOutPlayerIndex;
			}

			return player;
		}
		else
		{
			size_t nbItemPerThread = numPlayers / myNumComputeTasks;
			size_t itemIndex = 0;
			for (size_t i = 0; i < myNumComputeTasks; ++i)
			{
				ComputeTaskArg& arg = myComputeTaskArgs[i];
				arg.InitFindTask(aPlayerId, itemIndex, (i + 1) == myNumComputeTasks ? numPlayers : itemIndex + nbItemPerThread);
				itemIndex += nbItemPerThread;
			}

			myFindTaskPlayerFound = false;

			StartTasks(myNumComputeTasks);

			DWORD waitResult = WaitForSingleObject(myComputeTaskDoneEvent, INFINITE);

			if (myFindTaskPlayerFound)
			{
				aOutPlayerIndex = myFindTaskPlayerIndex;
				return myFindTaskPlayer;
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

		size_t playerIndex;
		Player* p = FindPlayer(aPlayerId, playerIndex);
		if (p != nullptr)
		{
			if (p->myIsAvailable)
				myCacheResultPlayer = nullptr;
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

		if (myNumPlayers % 100 == 0)
			printf("\t\t ===> num available players in system %u / %u\n", myAvailablePlayers, myNumPlayers);


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
			if (!p->myIsAvailable)
			{
				myCacheResultPlayer = nullptr;

				p->myIsAvailable = true;
				if (playerIndex > myAvailablePlayers)
				{
					std::swap(myPlayers[playerIndex], myPlayers[myAvailablePlayers]);
				}
				++myAvailablePlayers;
			}
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
			if (p->myIsAvailable)
			{
				myCacheResultPlayer = nullptr;

				p->myIsAvailable = false;
				if ((playerIndex + 1) < myAvailablePlayers)
				{
					std::swap(myPlayers[playerIndex], myPlayers[myAvailablePlayers - 1]);
				}
				--myAvailablePlayers;
			}
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
	MatchMaker::MatchMake(
		unsigned int	aPlayerId, 
		unsigned int	aPlayerIds[20], 
		int&			aOutNumPlayerIds)
	{
		MatchedResult& results = myComputeResults[0];

		MutexLock lock(myLock);

		if (myCacheResultPlayer != nullptr && myCacheResultPlayer->myPlayerId == aPlayerId)
		{
			++myCacheHitCount;
			if((myCacheHitCount % 10) == 0)
				printf("\n\t\t\t ==================> Cache hits : %u\n\n", myCacheHitCount);
		}
		else
		{
			size_t playerToMatchIndex;
			const Player* playerToMatch = FindPlayer(aPlayerId, playerToMatchIndex);

			if (!playerToMatch)
				return false;

			size_t numPlayers = myAvailablePlayers;

			if (numPlayers < 20 || numPlayers < myNumComputeTasks)
			{
				ComputeDistRange(*playerToMatch, playerToMatchIndex, 0, numPlayers, results);
				std::sort(results.myResults, results.myResults + results.myCount, MatchComp);
				if (results.myCount > 20)
				{
					results.myCount = 20;
				}
				myCacheResultPlayer = playerToMatch;
			}
			else
			{
				size_t nbItemPerThread = numPlayers / myNumComputeTasks;
				size_t itemIndex = 0;
				for (size_t i = 0; i < myNumComputeTasks; ++i)
				{
					ComputeTaskArg& arg = myComputeTaskArgs[i];
					arg.InitComputeTask(playerToMatch, playerToMatchIndex, itemIndex, (i + 1) == myNumComputeTasks ? numPlayers : itemIndex + nbItemPerThread);
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

			myCacheResultPlayer = playerToMatch;
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

		const Player* iterEnd = myPlayers + aEndIndex;
		const Player* player = myPlayers + aBeginIndex;
		Matched * target = &aOutResults.myResults[aOutResults.myCount];
		for (; player < iterEnd; ++player)
		{
			target->myId = player->myPlayerId;

			if (player == &aPlayerToMatch)
			{
				target->myDist = 0;
			}
			else
			{
				target->myDist = Dist(aPlayerToMatch.myPreferenceVector, player->myPreferenceVector);
			}
			++aOutResults.myCount;
			++target;
		}

		//std::sort(aOutResults.myResults, aOutResults.myResults + aOutResults.myCount, MatchComp);
	}

	MatchMaker::Player*
	MatchMaker::FindPlayerRange(
		unsigned int	aPlayerId,
		size_t			aBeginIndex,
		size_t			aEndIndex,
		size_t&			aOutPlayerIndex)
	{
		aOutPlayerIndex = aBeginIndex;
		Player* iterEnd = myPlayers + aEndIndex;
		Player* player = myPlayers + aBeginIndex;
		for (; !myFindTaskPlayerFound && player < iterEnd; ++player, ++aOutPlayerIndex)
		{
			if (player->myPlayerId == aPlayerId)
			{
				return player;
			}
		}

		return nullptr;
	}



