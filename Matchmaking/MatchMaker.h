#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#define MAX_NUM_PLAYERS (1000000)

#include "Mutex.h"
#include <crtdbg.h>

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

	class Player 
	{
	public:

		//Player(
		//	unsigned int	aPlayerId,
		//	float			aPreferenceVector[20],
		//	bool			aIsAvailable
		//)
		//	: myPlayerId(aPlayerId)
		//	, myPreferenceVector(new float[20])
		//	, myIsAvailable(aIsAvailable)
		//{
		//	SetPreferences(aPreferenceVector);
		//}

		Player()
		{ }

		~Player()
		{
		}

		void			SetPreferences(
							float aPreferenceVector[20])
		{
			memcpy(myPreferenceVector, aPreferenceVector, sizeof(float[20]));
		}

		unsigned int	myPlayerId; 
		float			myPreferenceVector[20]; 
		bool			myIsAvailable; 
	};

	class Matched
	{
	public:

		Matched()
		{
		}

		Matched& operator=(const Matched& other)
		{
			if (this == &other)
				return *this;
			myDist = other.myDist;
			myId = other.myId;
			return *this;
		}

		float			myDist;
		unsigned int	myId;
	};

	class ComputeTaskArg
	{
	public:

		enum class ETaskType
		{
			Reset,
			Compute
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


	static bool			MatchComp(
							Matched const&	aA,
							Matched const&	aB);


	Player*				FindPlayer(
							unsigned int	aPlayerId,
							size_t&			aOutIndex) const;

	static void			TaskHandler(
							void*			arg);

	void				RunTasks();


	const size_t		myMaxNumPlayers = MAX_NUM_PLAYERS;
	const size_t		myMaxNumDists = ((myMaxNumPlayers * (myMaxNumPlayers + 1)) / 2) + 1;
	const size_t		myMaxNumDistsPart1 = myMaxNumDists / 4;
	const size_t		myMaxNumDistsPart2 = myMaxNumDistsPart1 * 2;
	const size_t		myMaxNumDistsPart3 = myMaxNumDistsPart1 * 3;
	const size_t		myMaxNumDistsPart4 = myMaxNumDists - myMaxNumDistsPart3;


	inline size_t		CompDistIndex(
							size_t	i,
							size_t	j) const
	{
		if (j > i)
		{
			return (j * (j + 1) / 2) + i;
		}
		else
		{
			return (i * (i + 1) / 2) + j;
		}
	}

	inline float&		GetDist(
							size_t	i,
							size_t	j)
	{
		size_t index = CompDistIndex(i, j);
		_ASSERTE(index < myMaxNumDists);
		if (index < myMaxNumDistsPart1)
		{
			return myDists1[index];
		}
		else if(index < myMaxNumDistsPart2)
		{
			return myDists2[index - myMaxNumDistsPart1];
		}
		else if (index < myMaxNumDistsPart3)
		{
			return myDists3[index - myMaxNumDistsPart2];
		}
		else
		{
			return myDists4[index - myMaxNumDistsPart3];
		}

	}

	void				ComputeDistRange(
							const Player&	aPlayerToMatch,
							size_t			aPlayerToMatchIndex,
							size_t			aBeginIndex,
							size_t			aEndIndex);

	void				ResetOrComputeDist(
							const Player&	aPlayerToMatch,
							size_t			aPlayerToMatchIndex,
							size_t			aNumPlayers,
							ComputeTaskArg::ETaskType aTaskType);

	void				ResetDistRange(
							size_t			aPlayerToMatchIndex,
							size_t			aBeginIndex,
							size_t			aEndIndex);

	ComputeTaskArg*		myComputeTaskArgs;
	HANDLE*				myComputeTaskThreads;
	unsigned int		myNumComputeTasks;
	volatile LONG 		myComputeTaskArgIndex;
	volatile LONG 		myComputeTaskDoneCounter;
	HANDLE				myRunComputeTaskSem;
	HANDLE				myComputeTaskDoneEvent;

	float*				myDists1;
	float*				myDists2;
	float*				myDists3;
	float*				myDists4;
	Player				myPlayers[MAX_NUM_PLAYERS];
	size_t				myNumPlayers;
	Mutex				myLock;

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H