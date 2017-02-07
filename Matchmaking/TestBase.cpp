#include "stdafx.h"

#include "TestBase.h"

bool
Test::IsEqual(
	int				aThreadIndex,
	int				aTestIndex,
	const TestResult & a,
	const TestResult & b)
{
	bool isEqual = false;

	if (a.myPlayerId != b.myPlayerId)
	{
		printf("[Test::IsEqual - failed][ThreadIndex=%i,TestIndex=%i] myPlayerId inequality : %u != %u\n", aThreadIndex, aTestIndex, a.myPlayerId, b.myPlayerId);
		goto IsEqual_end;
	}

	if (a.myNumPlayers != b.myNumPlayers)
	{
		printf("[Test::IsEqual - failed][ThreadIndex=%i,TestIndex=%i] myNumPlayers inequality : %i != %i\n", aThreadIndex, aTestIndex, a.myNumPlayers, b.myNumPlayers);
		goto IsEqual_end;
	}

	for (auto i = 0; i < a.myNumPlayers; ++i)
	{
		if (a.myResultIds[i] != b.myResultIds[i])
		{
			printf("[Test::IsEqual - failed][ThreadIndex=%i,TestIndex=%i] myResultIds[%i] inequality : %u != %u\n", aThreadIndex, aTestIndex, i, a.myResultIds[i], b.myResultIds[i]);
			goto IsEqual_end;
		}
	}

	isEqual = true;

IsEqual_end:

	return isEqual;
}
