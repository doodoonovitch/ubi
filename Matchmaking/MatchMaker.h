#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#define MAX_NUM_PLAYERS (1000000)

#include "Mutex.h"

class MatchMaker
{
public:

	// keep these methods
	static MatchMaker&	GetInstance(); 

	bool				AddUpdatePlayer(
							unsigned int	aPlayerId, 
							float			aPreferenceVector[20]); 

	bool				SetPlayerAvailable(
							unsigned int	aPlayerId); 

	bool				SetPlayerUnavailable(
							unsigned int	aPlayerId); 

	bool				MatchMake(
							unsigned int	aPlayerId, 
							unsigned int	aPlayerIds[20], 
							int&			aOutNumPlayerIds); 

private: 

	// I don't care if you change anything below this 

	typedef unsigned char Preference;

	class Matched
	{
	public:

		Matched()
			: myDist(-1)
		{
		}

		int				myDist;
		unsigned int	myId;
	};

	class MatchedResult
	{
	public:

		MatchedResult()
			: myResults(nullptr)
			, myCount(0)
		{ }

		~MatchedResult()
		{
			delete[] myResults;
		}

		Matched*		myResults;
		size_t			myCount;
	};

	class Player 
	{
	public:

		Player()
			: myPlayerId(0)
		{ }

		~Player()
		{
			//delete [] myPreferenceVector; 
		}

		void			SetPreferences(
							Preference aPreferenceVector[20])
		{
			memcpy(myPreferenceVector, aPreferenceVector, sizeof(Preference[20]));
		}

		unsigned int	myPlayerId;
		Preference		myPreferenceVector[20];
		bool			myIsAvailable;
	};

	class ComputeTaskArg
	{
	public:

		enum class ETaskType
		{
			Compute,
			Exit
		};

		const Player*	myPlayerToMatch;
		size_t			myPlayerToMatchIndex;
		size_t			myBeginIndex;
		size_t			myEndIndex;
		ETaskType		myTaskType;

		void			Reset(
							const Player*	aPlayerToMatch,
							size_t			aPlayerToMatchIndex,
							size_t			aBeginIndex,
							size_t			aEndIndex,
							ETaskType		aTaskType);
	};

	static void			TaskHandler(
							void*			arg);

	void				RunTask();

	void				StartTasks(unsigned int taskCount);

	void				ComputeDistRange(
							const Player&	aPlayerToMatch,
							size_t			aPlayerToMatchIndex,
							size_t			aBeginIndex,
							size_t			aEndIndex,
							MatchedResult&	aOutResults);

	int					Dist(
		const Preference aA[20],
		const Preference aB[20]) const;

	static bool			MatchComp(
							const Matched&	aA,
							const Matched&	aB)
	{
		return (aA.myDist < aB.myDist);
	}

	Player*				FindPlayer(
							unsigned int	aPlayerId,
							size_t&			aOutPlayerIndex) const;

	int					myDistCache[256][256];
	MatchedResult*		myComputeResults;
	ComputeTaskArg*		myComputeTaskArgs;			// task pool 
	HANDLE*				myComputeTaskThreads;		// threads pool
	unsigned int		myNumComputeTasks;			// task pool count
	volatile LONG 		myComputeTaskArgIndex;		// next task pool to execute (if myComputeTaskArgIndex < myNumComputeTasks)
	volatile LONG 		myComputeTaskDoneCounter;	// count of executed tasks
	HANDLE				myRunComputeTaskSem;		// available tasks to execute (semaphore)
	HANDLE				myComputeTaskDoneEvent;		// all tasks has been done

	Mutex				myLock; 
	size_t				myNumPlayers; 
	size_t				myAvailablePlayers;
	Player				myPlayers[MAX_NUM_PLAYERS]; 

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H