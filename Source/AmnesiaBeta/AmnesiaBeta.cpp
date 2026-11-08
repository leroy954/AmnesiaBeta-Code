// Copyright Epic Games, Inc. All Rights Reserved.

#include "AmnesiaBeta.h"
#include "AmnesiaLevelJobs.h"
#include "Modules/ModuleManager.h"

/** Game module. In the editor it also watches for level building jobs */
class FAmnesiaBetaModule : public FDefaultGameModuleImpl
{
public:

	virtual void StartupModule() override
	{
#if WITH_EDITOR
		FAmnesiaLevelJobs::Start();
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		FAmnesiaLevelJobs::Stop();
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FAmnesiaBetaModule, AmnesiaBeta, "AmnesiaBeta" );

DEFINE_LOG_CATEGORY(LogAmnesiaBeta)
