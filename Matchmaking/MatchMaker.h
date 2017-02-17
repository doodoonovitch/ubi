#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#define MAX_NUM_PLAYERS (1000000)

#include "Mutex.h"
#include <intrin.h>

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

	typedef __declspec(align(32)) float PrefVec[24];

	__declspec(align(32)) class Player
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
			//delete [] myPreferenceVector; 
		}

		void			SetPreferences(
							float aPreferenceVector[20])
		{
			memcpy(myPreferenceVector, aPreferenceVector, sizeof(float[20]));
			memset(myPreferenceVector + 20, 0, sizeof(float[4]));
		}

		PrefVec			myPreferenceVector;
		unsigned int	myPlayerId;
		bool			myIsAvailable; 
	};

	static float
						Dist(
							const PrefVec aA,
							const PrefVec aB);


	Player*				FindPlayer(
							unsigned int	aPlayerId) const;


	Mutex				myLock; 
	int					myNumPlayers; 
	Player*				myPlayers; 

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H