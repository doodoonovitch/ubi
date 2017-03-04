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


	Player*				FindPlayer(
							unsigned int	aPlayerId) const;

	int					myDistCache[256][256];

	int					Dist(
							const Preference aA[20],
							const Preference aB[20]) const;

	static bool MatchComp(
							Matched*	aA,
							Matched*	aB);

	Mutex				myLock; 
	int					myNumPlayers; 
	Player				myPlayers[MAX_NUM_PLAYERS]; 

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H