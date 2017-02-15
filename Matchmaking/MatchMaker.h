#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#define MAX_NUM_PLAYERS (1000000)

#include "Mutex.h"

#include <algorithm>

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

	class Matched
	{
	public:

		Matched()
			: myDist(-1.f)
			, myId(-1)
		{
		}

		float			myDist;
		unsigned int	myId;
	};

	static bool
		MatchComp(
			Matched*	aA,
			Matched*	aB)
	{
		return (aA->myDist < aB->myDist);
	}


	class MatchedBinHeap
	{

	public:

		static const unsigned int MaxCapacity = 20;

		MatchedBinHeap(
			unsigned int	aCapacity = MaxCapacity)
			: myCapacity(aCapacity)
			, mySize(0)
		{
			for (unsigned int i = 0; i < myCapacity; ++i)
			{
				myArray[i] = &myStorage[i];
			}
		}

		void AddItem(
			unsigned int	aId,
			float			aDist)
		{
			if (mySize < myCapacity)
			{
				Matched* item = myArray[mySize];
				++mySize;

				item->myId = aId;
				item->myDist = aDist;

				std::push_heap(myArray, myArray + mySize, MatchComp);
			}
			else
			{
				Matched * currBiggest = myArray[0];
				if (currBiggest->myDist < aDist)
					return;

				currBiggest->myId = aId;
				currBiggest->myDist = aDist;

				//std::make_heap(myArray, myArray + mySize, MatchComp);
				SinkDown(1);
			}
		}

		void Export(
			unsigned int	aPlayerIds[20],
			int&			aOutNumPlayerIds)
		{
			aOutNumPlayerIds = mySize; 
			for(unsigned int j = 0; j < mySize; ++j)
				aPlayerIds[j] = myArray[j]->myId;
		}

	private:

		static unsigned int LeftChild(unsigned int index)
		{
			return index << 1;
		}

		static unsigned int RightChild(unsigned int index)
		{
			return (index << 1) + 1;
		}


		void SinkDown(unsigned int index)
		{
			if (index > mySize)
				return;

			unsigned int greatest = index;

			unsigned int childIndex = LeftChild(index);
			if (childIndex <= mySize)
			{
				if (MatchComp(myArray[greatest - 1], myArray[childIndex - 1]))
				{
					greatest = childIndex;
				}
			}

			childIndex = RightChild(index);
			if (childIndex <= mySize)
			{
				if (MatchComp(myArray[greatest - 1], myArray[childIndex - 1]))
				{
					greatest = childIndex;
				}
			}

			if (index != greatest)
			{
				std::swap(myArray[greatest - 1], myArray[index - 1]);
				SinkDown(greatest);
			}
		}

	private:

		unsigned int	myCapacity;
		unsigned int	mySize;
		Matched			myStorage[MaxCapacity];
		Matched*		myArray[MaxCapacity];
	};

	Player*				FindPlayer(
							unsigned int	aPlayerId) const;


	Mutex				myLock; 
	Mutex				myLockAdd;
	int					myNumPlayers; 
	Player*				myPlayers[MAX_NUM_PLAYERS]; 

						MatchMaker(); 

						~MatchMaker(); 
};

#endif // MATCHMAKER_H