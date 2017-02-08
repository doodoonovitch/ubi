#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#define MAX_NUM_PLAYERS (1000000)

#include "Mutex.h"
#include <map>


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

		Player(
			unsigned int	aPlayerId,
			float			aPreferenceVector[20],
			bool			aIsAvailable
		)
			: myPlayerId(aPlayerId)
			, myPreferenceVector(new float[20])
			, myIsAvailable(aIsAvailable)
		{
			SetPreferences(aPreferenceVector);
		}

		~Player()
		{
			delete [] myPreferenceVector; 
		}

		void			SetPreferences(
							float aPreferenceVector[20])
		{
			memcpy(myPreferenceVector, aPreferenceVector, sizeof(float[20]));
		}

		unsigned int	myPlayerId; 
		float*			myPreferenceVector; 
		bool			myIsAvailable; 
	};

	typedef std::map<unsigned int, Player*> PlayerMap;

	Player*				FindPlayer(
							unsigned int	aPlayerId) const;


	Mutex				myLock; 
	int					myNumPlayers; 
	PlayerMap			myPlayersMap;
	Player*				myPlayers[MAX_NUM_PLAYERS];

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H