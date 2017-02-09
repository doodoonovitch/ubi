#include "stdafx.h"

#include "MatchMaker.h"
#include "TestBase.h"

#include <string>
#include <algorithm>

	class QPTimer 
	{
	public: 

		QPTimer()
		{
			LARGE_INTEGER freq; 
			QueryPerformanceFrequency(&freq); 

			myInvFreq = 1000000.0 / (double) freq.QuadPart; 
		}

		void
		Start()
		{
			QueryPerformanceCounter(&myStart); 
		}

		// returns micro seconds
		double
		Get()
		{
			QueryPerformanceCounter(&myStop);

			double t = (double)(myStop.QuadPart - myStart.QuadPart) * myInvFreq;

			myStart = myStop; 

			return t; 
		}

		LARGE_INTEGER myStart; 
		LARGE_INTEGER myStop; 
		double myInvFreq; 
	};

	class ScopedQPTimer
	{
	public: 

		ScopedQPTimer(
			const char*		aMsg = NULL)
		{
			myMsg = aMsg; 
			myTimer.Start(); 
		}

		~ScopedQPTimer()
		{
			double used = myTimer.Get(); 
			printf("%s %f\n", myMsg, used / 1000.0); 
		}

		QPTimer		myTimer; 
		const char*	myMsg; 
	}; 

	#define USE_PREDICTABLE_RANDOMNESS

	unsigned int 
	RandomUInt32()
	{
	#ifdef USE_PREDICTABLE_RANDOMNESS

		return (((unsigned int) rand()) << 16) | rand(); 

	#else

		// better random function 
		unsigned int r; 
		rand_s(&r);
		return r; 

	#endif
	}

	float 
	RandomFloat32()
	{
		return (float) RandomUInt32() / (float) 0xFFFFFFFF; 
	}

#ifdef USE_PREDICTABLE_RANDOMNESS
	#define TEST_SUITE_PREFIX_1 "Rand_"
#else
	#define TEST_SUITE_PREFIX_1 "SRand_"
#endif // USE_PREDICTABLE_RANDOMNESS

//#define PLAYER_COUNT_5K
#ifdef PLAYER_COUNT_5K
	#define PLAYER_COUNT 5000;
	#define TEST_SUITE_PREFIX_2 "5K_"
#else
	#define PLAYER_COUNT 100000;
	#define TEST_SUITE_PREFIX_2 "100K_"
#endif // PLAYER_COUNT_5K


	static const unsigned int PlayerCount = PLAYER_COUNT;
	static const unsigned int TestThreadCount = Test::TestThreadCount;


//#define GENERATE_TEST_SAMPLE 1
#ifdef GENERATE_TEST_SAMPLE
	#include "GenerateTestSamples.h"
	static GenerateTestSamples generateTestSamples(TEST_SUITE_PREFIX_1 TEST_SUITE_PREFIX_2 "TestSuite");
#else
	//#define USE_TEST_SUITES
	#ifdef USE_TEST_SUITES

	#ifdef USE_PREDICTABLE_RANDOMNESS
		#ifdef PLAYER_COUNT_5K
		#include "TestSamples\Rand_5K_TestSuite_MainData.h"
		#else
		#include "TestSamples\Rand_100K_TestSuite_MainData.h"
		#endif // PLAYER_COUNT_5K
	#else
		#ifdef PLAYER_COUNT_5K
		#include "TestSamples\SRand_5K_TestSuite_MainData.h"
		#else
		#include "TestSamples\SRand_100K_TestSuite_MainData.h"
		#endif // PLAYER_COUNT_5K
	#endif // USE_PREDICTABLE_RANDOMNESS

	static Test::PlayerData * testSuitePlayerData[Test::TestThreadCount][Test::TestPerThreadCount];
	static Test::TestResult * testSuiteResults[Test::TestThreadCount][Test::TestPerThreadCount];

	void InitializeTestSuiteResult()
	{
		for (auto i = 0; i < Test::TestPerThreadCount; ++i)
		{
			testSuitePlayerData[0][i] = &testPlayerData_0[i];
			testSuitePlayerData[1][i] = &testPlayerData_1[i];
			testSuitePlayerData[2][i] = &testPlayerData_2[i];
			testSuitePlayerData[3][i] = &testPlayerData_3[i];
			testSuitePlayerData[4][i] = &testPlayerData_4[i];
			testSuitePlayerData[5][i] = &testPlayerData_5[i];
			testSuitePlayerData[6][i] = &testPlayerData_6[i];
			testSuitePlayerData[7][i] = &testPlayerData_7[i];
			testSuitePlayerData[8][i] = &testPlayerData_8[i];
			testSuitePlayerData[9][i] = &testPlayerData_9[i];
			testSuitePlayerData[10][i] = &testPlayerData_10[i];
			testSuitePlayerData[11][i] = &testPlayerData_11[i];
			testSuitePlayerData[12][i] = &testPlayerData_12[i];
			testSuitePlayerData[13][i] = &testPlayerData_13[i];
			testSuitePlayerData[14][i] = &testPlayerData_14[i];
			testSuitePlayerData[15][i] = &testPlayerData_15[i];

			testSuiteResults[0][i] = &testResults_0[i];
			testSuiteResults[1][i] = &testResults_1[i];
			testSuiteResults[2][i] = &testResults_2[i];
			testSuiteResults[3][i] = &testResults_3[i];
			testSuiteResults[4][i] = &testResults_4[i];
			testSuiteResults[5][i] = &testResults_5[i];
			testSuiteResults[6][i] = &testResults_6[i];
			testSuiteResults[7][i] = &testResults_7[i];
			testSuiteResults[8][i] = &testResults_8[i];
			testSuiteResults[9][i] = &testResults_9[i];
			testSuiteResults[10][i] = &testResults_10[i];
			testSuiteResults[11][i] = &testResults_11[i];
			testSuiteResults[12][i] = &testResults_12[i];
			testSuiteResults[13][i] = &testResults_13[i];
			testSuiteResults[14][i] = &testResults_14[i];
			testSuiteResults[15][i] = &testResults_15[i];
		}
	}

	#endif // USE_TEST_SUITES
#endif // GENERATE_TEST_SAMPLE

	class RequestThread;

namespace
{
	void 
	Run(
		void*	aData)
	{
#ifdef USE_TEST_SUITES
		int testSampleId = reinterpret_cast<int>(aData);

		for (int testSample = 0; testSample < Test::TestPerThreadCount; ++testSample)
		{
			Test::PlayerData * player = testSuitePlayerData[testSampleId][testSample];
			Test::TestResult * expectedResult = testSuiteResults[testSampleId][testSample];

			MatchMaker::GetInstance().AddUpdatePlayer(player->myPlayerId, player->myPreferenceVector);

			if (player->myOffLineProbability < 0.05f)
				MatchMaker::GetInstance().SetPlayerAvailable(player->myPlayerId);
			else
				MatchMaker::GetInstance().SetPlayerUnavailable(player->myPlayerId);

			Test::TestResult result;
			result.myPlayerId = player->myPlayerId;

			memset(result.myResultIds, -1, sizeof(result.myResultIds));
			{
				ScopedQPTimer timer("matching time in milliseconds");
				MatchMaker::GetInstance().MatchMake(result.myPlayerId, result.myResultIds, result.myNumPlayers);
			}

			using std::sort;
			if (result.myNumPlayers > 0)
			{
				sort(result.myResultIds, result.myResultIds + result.myNumPlayers);
			}
			if (expectedResult->myNumPlayers > 0)
			{
				sort(expectedResult->myResultIds, expectedResult->myResultIds + expectedResult->myNumPlayers);
			}
			Test::IsEqual(testSampleId, testSample, result, *expectedResult);
		}
#else

	#ifdef GENERATE_TEST_SAMPLE
		int testSampleId = reinterpret_cast<int>(aData);

		generateTestSamples.BeginGenerateTestResult(testSampleId);

		for(int testSample = 0; testSample < Test::TestPerThreadCount; ++testSample)
	#else
		for(;;)
	#endif // GENERATE_TEST_SAMPLE
		{
			// add or update a random player to the system 
			float preferenceVector[20]; 
			for(int i = 0; i < 20; i++)
				preferenceVector[i] = RandomFloat32(); 

			unsigned int playerId = RandomUInt32() % PlayerCount;
			MatchMaker::GetInstance().AddUpdatePlayer(playerId, preferenceVector); 

			// players goes on-line / off-line all the time 
			float offLineProbability = RandomFloat32();
			if(offLineProbability < 0.05f)
				MatchMaker::GetInstance().SetPlayerAvailable(playerId); 
			else 
				MatchMaker::GetInstance().SetPlayerUnavailable(playerId); 

			// match make a player
			unsigned int ids[20]; 
			int numPlayers; 

			{
				ScopedQPTimer timer("matching time in milliseconds");
				MatchMaker::GetInstance().MatchMake(playerId, ids, numPlayers);
			}

	#ifdef GENERATE_TEST_SAMPLE
			generateTestSamples.GenerateTestResult(testSampleId, playerId, preferenceVector, offLineProbability, ids, numPlayers);
	#endif // GENERATE_TEST_SAMPLE
		}

	#ifdef GENERATE_TEST_SAMPLE
		generateTestSamples.EndGenerateTestResult(testSampleId);
	#endif // GENERATE_TEST_SAMPLE
#endif // USE_TEST_SUITES
	}
}

class RequestThread 
{
public: 

	RequestThread(int aTestSampleId)
		: myTestSampleId(aTestSampleId)
	{

	}

	~RequestThread()
	{
	}

	uintptr_t
	Start()
	{
		uintptr_t handle = _beginthread(Run, 0, (void*)myTestSampleId);
		return handle;
	}

	int myTestSampleId;
};


int main(int argc, char* argv[])
{
	MatchMaker::GetInstance(); 

#ifdef GENERATE_TEST_SAMPLE
	generateTestSamples.BeginGenerateMainData();
#else
	#ifdef USE_TEST_SUITES

	InitializeTestSuiteResult();

	#endif // USE_TEST_SUITES
#endif // GENERATE_TEST_SAMPLE


#ifdef USE_TEST_SUITES

	for (auto i = 0; i < PlayerCount; i++)
	{
		Test::PlayerData & player = mainDataSample[i];
		MatchMaker::GetInstance().AddUpdatePlayer(player.myPlayerId, player.myPreferenceVector);
		if (player.myOffLineProbability < 0.05f)
			MatchMaker::GetInstance().SetPlayerAvailable(player.myPlayerId);
		else
			MatchMaker::GetInstance().SetPlayerUnavailable(player.myPlayerId);
	}

#else

	// add 100000 players to the system before starting any request threads 
	for(auto i = 0; i < PlayerCount; i++)
	{
		float preferenceVector[20]; 
		for(int i = 0; i < 20; i++)
			preferenceVector[i] = RandomFloat32(); 

		unsigned int playerId = RandomUInt32() % PlayerCount;

		// players goes on-line / off-line all the time 
		float offLineProbability = RandomFloat32();

#ifdef GENERATE_TEST_SAMPLE
		generateTestSamples.GenerateMainDataItem(playerId, preferenceVector, offLineProbability);
#endif // GENERATE_TEST_SAMPLE

		MatchMaker::GetInstance().AddUpdatePlayer(playerId, preferenceVector); 
		if (offLineProbability < 0.05f)
			MatchMaker::GetInstance().SetPlayerAvailable(playerId);
		else
			MatchMaker::GetInstance().SetPlayerUnavailable(playerId);
	}

#endif // USE_TEST_SUITES

#if defined(GENERATE_TEST_SAMPLE) || defined(USE_TEST_SUITES)

#ifdef GENERATE_TEST_SAMPLE
	generateTestSamples.EndGenerateMainData();
#endif // GENERATE_TEST_SAMPLE

	for (auto i = 0; i < TestThreadCount; i++)
	{
		Run((void*)i);
	}
	
#ifdef USE_TEST_SUITES
	printf("[Enter] to terminate...");
	getchar();
#endif

#else

	printf("starting worker threads\n"); 

	for (auto i = 0; i < TestThreadCount; i++)
		(new RequestThread(i))->Start();

	Sleep(-1);

#endif // defined(GENERATE_TEST_SAMPLE) || defined(USE_TEST_SUITE)

	return 0;
}

