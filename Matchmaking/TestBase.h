#ifndef TESTBASE_H
#define TESTBASE_H

#include <vector>

class Test
{
public:

	static const int			TestPerThreadCount = 2;
	static const unsigned int	TestThreadCount = 16;

	class PlayerData
	{
	public:
		unsigned int	myPlayerId;
		float			myPreferenceVector[20];
		float			myOffLineProbability;
	};

	class TestResult
	{
	public:
		unsigned int	myPlayerId;
		unsigned int	myResultIds[20];
		int				myNumPlayers;
	};

	static bool			IsEqual(
							int				aThreadIndex,
							int				aTestIndex,
							const TestResult & a, 
							const TestResult & b);
};


#endif // TESTBASE_H