#ifndef GENERATETESTSAMPLES_H
#define GENERATETESTSAMPLES_H

#include <string>
#include "TestBase.h"
#include "Mutex.h"


class GenerateTestSamples
{
public:

	void				BeginGenerateMainData();
	void				EndGenerateMainData();
	void				GenerateMainDataItem(
							unsigned int	aPlayerId,
							float			aPreferenceVector[20],
							float			aOnlineProbability);

	void				BeginGenerateTestResult(
							int				aTestSampleId);

	void				EndGenerateTestResult(
							int				aTestSampleId);
	void				GenerateTestResult(
							int				aTestSampleId,
							unsigned int	aPlayerId,
							float			aPreferenceVector[20],
							float			aOnlineProbability,
							unsigned int	aResultIds[20],
							int				myNumPlayers);


	GenerateTestSamples(
							const std::string & filenameBase);

	~GenerateTestSamples();

	FILE*				myMainDataSampleStream;
	FILE*				myTestPlayerDataStreams[Test::TestThreadCount];
	FILE*				myTestResultStreams[Test::TestThreadCount];
	Mutex				myTestResultFileMutex[Test::TestThreadCount];

private:

	void				PrintPlayerData(
							FILE*			stream,
							unsigned int	aPlayerId,
							float			aPreferenceVector[20],
							float			aOnlineProbability);

	void				PrintPreferenceVector(
							FILE*			stream, 
							float			aPreferenceVector[20]);

	void				PrintTestPlayerData(
							int				aTestSampleId,
							unsigned int	aPlayerId,
							float			aPreferenceVector[20],
							float			aOnlineProbability);
	void				PrintTestResult(
							int				aTestSampleId,
							unsigned int	aPlayerId,
							unsigned int	aResultIds[20],
							int				myNumPlayers);
};

#endif // GENERATETESTSAMPLES_H