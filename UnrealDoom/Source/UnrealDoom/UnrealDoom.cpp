// Copyright (c) 2024 UnrealDoom Contributors. All Rights Reserved.

#include "UnrealDoom.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogUnrealDoom);

void FUnrealDoomModule::StartupModule()
{
	UE_LOG(LogUnrealDoom, Log, TEXT("UnrealDoom module starting up."));
}

void FUnrealDoomModule::ShutdownModule()
{
	UE_LOG(LogUnrealDoom, Log, TEXT("UnrealDoom module shutting down."));
}

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealDoomModule, UnrealDoom, "UnrealDoom");
