#ifndef TESTBASE_H
#define TESTBASE_H

#include <vector>

namespace Test
{
	static const int			TestPerThreadCount = 2;
	static const unsigned int	TestThreadCount = 16;

	class PlayerData
	{
	public:
		unsigned int	myPlayerId;
		float			myPreferenceVector[20];
		float			myOnlineProbability;
	};

	class TestResult
	{
	public:
		unsigned int	myPlayerId;
		unsigned int	myResultIds[20];
		int				myNumPlayers;
	};
};


#endif // TESTBASE_H