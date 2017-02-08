#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#define MAX_NUM_PLAYERS (1000000)

#include "Mutex_0.h"

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

		Player()
		{
			myPreferenceVector = new float[20]; 
		}

		~Player()
		{
			delete [] myPreferenceVector; 
		}

		unsigned int	myPlayerId; 
		float*			myPreferenceVector; 
		bool			myIsAvailable; 
	};

	Mutex				myLock; 
	int					myNumPlayers; 
	Player*				myPlayers[MAX_NUM_PLAYERS]; 

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H