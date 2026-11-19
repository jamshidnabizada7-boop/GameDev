#include "TacticalCQBSystem.h"

DEFINE_LOG_CATEGORY(LogTacticalCQB);

void FTacticalCQBSystemModule::StartupModule()
{
	UE_LOG(LogTacticalCQB, Log, TEXT("TacticalCQBSystem module started."));
}

void FTacticalCQBSystemModule::ShutdownModule()
{
	UE_LOG(LogTacticalCQB, Log, TEXT("TacticalCQBSystem module shut down."));
}

IMPLEMENT_MODULE(FTacticalCQBSystemModule, TacticalCQBSystem)
