#include "stdafx.h"

#include "GenerateTestSamples.h"

#include <stdio.h>

GenerateTestSamples::GenerateTestSamples(
	const std::string & filenameBase)
	: myMainDataSampleStream(nullptr)
{
	std::string filePathBase = "TestSamples\\" + filenameBase;

	std::string mainDataSampleFilename = filePathBase + "_MainData.h";
	fopen_s(&myMainDataSampleStream, mainDataSampleFilename.c_str(), "w");

	for (auto i = 0; i < Test::TestThreadCount; ++i)
	{
		char tmp[80];
		sprintf_s(tmp, "_Results_%i.h", i);
		std::string resultFilename = filePathBase + tmp;
		fopen_s(&myTestResultStreams[i], resultFilename.c_str(), "w");

		sprintf_s(tmp, "_PlayerData_%i.h", i);
		std::string playerDataFilename = filePathBase + tmp;
		fopen_s(&myTestPlayerDataStreams[i], playerDataFilename.c_str(), "w");

		fprintf_s(myMainDataSampleStream, "#include \"%s\"\n", resultFilename.c_str());
		fprintf_s(myMainDataSampleStream, "#include \"%s\"\n", playerDataFilename.c_str());
	}
}

GenerateTestSamples::~GenerateTestSamples()
{
	fclose(myMainDataSampleStream);
	for (auto i = 0; i < Test::TestThreadCount; ++i)
	{
		fclose(myTestResultStreams[i]);
		fclose(myTestPlayerDataStreams[i]);
	}
}

void
GenerateTestSamples::PrintPlayerData(
	FILE*			stream,
	unsigned int	aPlayerId,
	float			aPreferenceVector[20],
	float			aOnlineProbability)
{
	fprintf_s(stream, "{ ");

	fprintf_s(stream, "%u, ", aPlayerId);

	PrintPreferenceVector(stream, aPreferenceVector);

	fprintf_s(stream, ", %ff ", aOnlineProbability);

	fprintf_s(stream, "}");
}

void 
GenerateTestSamples::PrintPreferenceVector(
	FILE *			stream, 
	float			aPreferenceVector[20])
{
	fprintf_s(stream, "{ ");
	for (int i = 0; i < 20; ++i)
	{
		if (i == 0)
		{
			fprintf_s(stream, "%ff", aPreferenceVector[i]);
		}
		else
		{
			fprintf_s(stream, ", %ff", aPreferenceVector[i]);
		}
	}
	fprintf_s(stream, "}");
}


void
GenerateTestSamples::BeginGenerateMainData()
{
	fprintf_s(myMainDataSampleStream, "static Test::PlayerData mainDataSample[] = \n{\n");
}

void
GenerateTestSamples::EndGenerateMainData()
{
	fprintf_s(myMainDataSampleStream, "};\n\n");
}

void
GenerateTestSamples::GenerateMainDataItem(
	unsigned int	aPlayerId,
	float			aPreferenceVector[20],
	float			aOnlineProbability)
{
	PrintPlayerData(myMainDataSampleStream, aPlayerId, aPreferenceVector, aOnlineProbability);
	fprintf_s(myMainDataSampleStream, ",\n");
}


void
GenerateTestSamples::PrintTestPlayerData(
	int				aTestSampleId,
	unsigned int	aPlayerId,
	float			aPreferenceVector[20],
	float			aOnlineProbability)
{
	FILE* stream = myTestPlayerDataStreams[aTestSampleId];
	PrintPlayerData(stream, aPlayerId, aPreferenceVector, aOnlineProbability);
}

void
GenerateTestSamples::PrintTestResult(
	int				aTestSampleId,
	unsigned int	aPlayerId,
	unsigned int	aResultIds[20],
	int				aNumPlayers)
{
	FILE* stream = myTestResultStreams[aTestSampleId];

	fprintf_s(stream, "{ %u, {", aPlayerId);
	for (auto i = 0; i < 20; ++i)
	{
		if (i == 0)
		{
			fprintf_s(stream, "%u", aResultIds[i]);
		}
		else
		{
			fprintf_s(stream, ", %u", aResultIds[i]);
		}
	}
	fprintf_s(stream, " }, %i }", aNumPlayers);
}

void
GenerateTestSamples::BeginGenerateTestResult(
	int				aTestSampleId)
{
	MutexLock lock(myTestResultFileMutex[aTestSampleId]);
	fprintf_s(myTestPlayerDataStreams[aTestSampleId], "static Test::PlayerData testPlayerData_%i[] = \n{\n", aTestSampleId);
	fprintf_s(myTestResultStreams[aTestSampleId], "static Test::TestResult testResults_%i[] = \n{\n", aTestSampleId);
}

void
GenerateTestSamples::EndGenerateTestResult(
	int				aTestSampleId)
{
	MutexLock lock(myTestResultFileMutex[aTestSampleId]);
	fprintf_s(myTestPlayerDataStreams[aTestSampleId], "};\n\n");
	fprintf_s(myTestResultStreams[aTestSampleId], "};\n\n");
}

void
GenerateTestSamples::GenerateTestResult(
	int				aTestSampleId,
	unsigned int	aPlayerId,
	float			aPreferenceVector[20],
	float			aOnlineProbability,
	unsigned int	aResultIds[20],
	int				myNumPlayers)
{
	MutexLock lock(myTestResultFileMutex[aTestSampleId]);

	PrintTestPlayerData(aTestSampleId, aPlayerId, aPreferenceVector, aOnlineProbability);
	fprintf_s(myTestPlayerDataStreams[aTestSampleId], ",\n");

	PrintTestResult(aTestSampleId, aPlayerId, aResultIds, myNumPlayers);
	fprintf_s(myTestResultStreams[aTestSampleId], ",\n");
}
