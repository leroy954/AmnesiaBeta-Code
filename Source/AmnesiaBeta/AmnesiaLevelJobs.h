#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"

#if WITH_EDITOR

/**
 *  Editor only helper that builds level content from JSON job files.
 *  A job file dropped in Saved/LevelJobs is executed in the editor world and answered with a result file.
 *  It exists so big levels can be filled with thousands of actors in one go.
 */
class FAmnesiaLevelJobs
{
public:

	/** Starts watching the job folder */
	static void Start();

	/** Stops watching the job folder */
	static void Stop();

private:

	/** Ticker callback. Runs a pending job if there is one */
	static bool Tick(float DeltaTime);

	/** Runs one job file and returns the result text */
	static FString RunJob(const FString& JobText);

	/** Ticker callback. Picks up cmd.json, which runs console commands and samples frame times. Works while the game is playing too */
	static bool TickCommands(float DeltaTime);

	/** Writes cmd_result.json for the running command job */
	static void FinishCommandJob();

	static FTSTicker::FDelegateHandle TickerHandle;
	static FTSTicker::FDelegateHandle CommandTickerHandle;
};

#endif
