#include "stdafx.h"

#include "MatchMaker.h"
#include "TestBase.h"

#include <string>

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
	#define USE_TEST_SUITES
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

	#endif // USE_TEST_SUITES
#endif // GENERATE_TEST_SAMPLE

	class RequestThread;

namespace
{
	void 
	Run(
		void*	aData)
	{
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
			float onlineProbability = RandomFloat32();
			if(onlineProbability < 0.05f)
				MatchMaker::GetInstance().SetPlayerAvailable(playerId); 
			else 
				MatchMaker::GetInstance().SetPlayerUnavailable(playerId); 

			// match make a player
			unsigned int ids[20]; 
			int numPlayers; 

			ScopedQPTimer timer("matching time in milliseconds"); 
			MatchMaker::GetInstance().MatchMake(playerId, ids, numPlayers); 

#ifdef GENERATE_TEST_SAMPLE
			generateTestSamples.GenerateTestResult(testSampleId, playerId, preferenceVector, onlineProbability, ids, numPlayers);
#endif // GENERATE_TEST_SAMPLE
		}

#ifdef GENERATE_TEST_SAMPLE
		generateTestSamples.EndGenerateTestResult(testSampleId);
#endif // GENERATE_TEST_SAMPLE
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
#endif // GENERATE_TEST_SAMPLE

	// add 100000 players to the system before starting any request threads 
	for(auto i = 0; i < PlayerCount; i++)
	{
		float preferenceVector[20]; 
		for(int i = 0; i < 20; i++)
			preferenceVector[i] = RandomFloat32(); 

		unsigned int playerId = RandomUInt32() % PlayerCount;

#ifdef GENERATE_TEST_SAMPLE
		generateTestSamples.GenerateMainDataItem(playerId, preferenceVector, 1.f);
#endif // GENERATE_TEST_SAMPLE

		MatchMaker::GetInstance().AddUpdatePlayer(playerId, preferenceVector); 
	}

#ifdef GENERATE_TEST_SAMPLE
	generateTestSamples.EndGenerateMainData();
#endif // GENERATE_TEST_SAMPLE

	printf("starting worker threads\n"); 

	RequestThread* testThreads[TestThreadCount];
	HANDLE threadHandles[TestThreadCount];

	for (auto i = 0; i < TestThreadCount; i++)
	{
		RequestThread * thread = new RequestThread(i);
		testThreads[i] = thread;
		threadHandles[i] = (HANDLE)thread->Start();
	}

	WaitForMultipleObjects(TestThreadCount, threadHandles, TRUE, INFINITE);

	for (auto i = 0; i < TestThreadCount; i++)
	{
		delete testThreads[i];
	}
	
	return 0;
}

