// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.

#include "DoomGameMode.h"
#include "DoomPlayerController.h"
#include "DoomPlayerPawn.h"
#include "DoomLevelActor.h"
#include "DoomSpecials.h"
#include "WAD/WadFile.h"
#include "GameLogic/DoomGameState.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomGameMode, Log, All);

// =============================================================================
// Constructor
// =============================================================================

ADoomGameMode::ADoomGameMode()
{
	// Enable ticking for the game loop
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// Set default classes for DOOM gameplay
	DefaultPawnClass = ADoomPlayerPawn::StaticClass();
	PlayerControllerClass = ADoomPlayerController::StaticClass();

	// DOOM is single-player by default
	bPauseable = true;
}

// =============================================================================
// AGameModeBase overrides
// =============================================================================

void ADoomGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UE_LOG(LogDoomGameMode, Log, TEXT("ADoomGameMode::InitGame - Initializing DOOM systems"));

	// Initialize the math lookup tables (sine, cosine, tangent)
	FDoomMath::InitTables();

	// Create all DOOM subsystem objects
	CreateSubsystems();

	// Check for WAD path from command line: -wadfile=<path>
	FString CmdWadPath;
	if (FParse::Value(FCommandLine::Get(), TEXT("-wadfile="), CmdWadPath))
	{
		WadFilePath = CmdWadPath;
		UE_LOG(LogDoomGameMode, Log, TEXT("WAD path from command line: %s"), *WadFilePath);
	}

	// If we have a WAD path, initialize DOOM
	if (!WadFilePath.IsEmpty())
	{
		if (!InitializeDoom(WadFilePath))
		{
			ErrorMessage = FString::Printf(TEXT("Failed to initialize DOOM with WAD: %s"), *WadFilePath);
			UE_LOG(LogDoomGameMode, Error, TEXT("%s"), *ErrorMessage);
		}
	}
	else
	{
		UE_LOG(LogDoomGameMode, Warning, TEXT("No WAD file path specified. Set WadFilePath or use -wadfile= command line option."));
	}
}

void ADoomGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogDoomGameMode, Log, TEXT("ADoomGameMode::StartPlay"));

	if (bDoomInitialized)
	{
		// Start with the demo screen / title sequence
		if (DoomState)
		{
			DoomState->SetGameState(EDoomGameState::DemoScreen);
			DoomState->SetGameAction(EDoomGameAction::Nothing);
		}

		UE_LOG(LogDoomGameMode, Log, TEXT("DOOM engine started. Awaiting game start command."));
	}
}

void ADoomGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDoomInitialized)
	{
		return;
	}

	// Accumulate real time and run DOOM tics at the fixed 35 Hz rate.
	// This is equivalent to TryRunTics from d_net.c.
	AccumulatedTime += DeltaSeconds;

	// Cap accumulated time to prevent spiral of death (max 3 tics per frame)
	constexpr float MaxAccumulated = SecondsPerTic * 3.0f;
	if (AccumulatedTime > MaxAccumulated)
	{
		AccumulatedTime = MaxAccumulated;
	}

	// Run tics
	while (AccumulatedTime >= SecondsPerTic)
	{
		AccumulatedTime -= SecondsPerTic;
		RunSingleTic();
	}
}

// =============================================================================
// DOOM initialization
// =============================================================================

bool ADoomGameMode::InitializeDoom(const FString& WadPath)
{
	UE_LOG(LogDoomGameMode, Log, TEXT("Initializing DOOM with WAD: %s"), *WadPath);

	if (!WadManager)
	{
		UE_LOG(LogDoomGameMode, Error, TEXT("WadManager is null - subsystems not created"));
		return false;
	}

	// Step 1: Open the WAD file
	if (!WadManager->OpenWadFile(WadPath))
	{
		UE_LOG(LogDoomGameMode, Error, TEXT("Failed to open WAD file: %s"), *WadPath);
		return false;
	}

	UE_LOG(LogDoomGameMode, Log, TEXT("WAD loaded: %d lumps"), WadManager->GetNumLumps());

	// Step 2: Detect the game mode from WAD contents
	DetectGameMode();

	// Step 3: Initialize the game state with defaults
	if (DoomState)
	{
		DoomState->Initialize();
		DoomState->SetConsolePlayer(0);
		DoomState->SetDisplayPlayer(0);
		DoomState->SetPlayerInGame(0, true);
		DoomState->SetPrecache(true);
	}

	// Step 4: Initialize entity info tables
	UDoomEntityInfo::InitMobjInfoTable();

	WadFilePath = WadPath;
	bDoomInitialized = true;

	UE_LOG(LogDoomGameMode, Log, TEXT("DOOM initialized successfully. Game mode: %d"),
		static_cast<int32>(DoomState ? DoomState->GetGameMode() : EDoomGameMode::Indetermined));

	return true;
}

// =============================================================================
// Game flow
// =============================================================================

void ADoomGameMode::StartNewGame(EDoomSkill Skill, int32 Episode, int32 Map)
{
	if (!bDoomInitialized)
	{
		UE_LOG(LogDoomGameMode, Error, TEXT("Cannot start game - DOOM not initialized"));
		return;
	}

	UE_LOG(LogDoomGameMode, Log, TEXT("Starting new game: Skill=%d Episode=%d Map=%d"),
		static_cast<int32>(Skill), Episode, Map);

	// Validate episode/map based on game mode
	if (DoomState)
	{
		const EDoomGameMode GameMode = DoomState->GetGameMode();

		// Clamp episode for shareware
		if (GameMode == EDoomGameMode::Shareware && Episode > 1)
		{
			Episode = 1;
			UE_LOG(LogDoomGameMode, Warning, TEXT("Shareware only supports Episode 1"));
		}

		// Set up game state for new game (equivalent to G_InitNew)
		DoomState->SetGameSkill(Skill);
		DoomState->SetGameEpisode(Episode);
		DoomState->SetGameMap(Map);
		DoomState->SetRespawnMonsters(Skill == EDoomSkill::Nightmare);
		DoomState->SetPaused(false);
		DoomState->SetDemoPlayback(false);
		DoomState->SetDemoRecording(false);

		// Reset level time
		DoomState->SetLevelTime(0);
		DoomState->SetLevelStartTic(DoomState->GetGameTic());

		// Set action to trigger level load on next tic
		DoomState->SetGameAction(EDoomGameAction::LoadLevel);
		DoomState->SetGameState(EDoomGameState::Level);
	}

	// Load the first level
	LoadLevel(Episode, Map);
}

void ADoomGameMode::LoadLevel(int32 Episode, int32 Map)
{
	if (!bDoomInitialized)
	{
		UE_LOG(LogDoomGameMode, Error, TEXT("Cannot load level - DOOM not initialized"));
		return;
	}

	const FString MapName = BuildMapLumpName(Episode, Map);
	UE_LOG(LogDoomGameMode, Log, TEXT("Loading level: %s"), *MapName);

	// Step 1: Clean up any existing level
	CleanupCurrentLevel();

	// Step 2: Find the map lump in the WAD
	const int32 MapLumpIndex = WadManager->FindLump(MapName);
	if (MapLumpIndex < 0)
	{
		UE_LOG(LogDoomGameMode, Error, TEXT("Map lump '%s' not found in WAD"), *MapName);
		return;
	}

	// Step 3: Spawn the level geometry actor
	SpawnLevelActor();

	// Step 4: Reset thinker list for the new level
	// ThinkerManager->RemoveAllThinkers(); // To be implemented

	// Step 5: Reset level statistics
	if (DoomState)
	{
		DoomState->SetTotalKills(0);
		DoomState->SetTotalItems(0);
		DoomState->SetTotalSecrets(0);
		DoomState->SetLevelTime(0);
	}

	// Step 6: Initialize specials (doors, platforms, lights) for this level
	if (Specials)
	{
		Specials->SpawnSpecials();
	}

	bInLevel = true;

	UE_LOG(LogDoomGameMode, Log, TEXT("Level %s loaded successfully"), *MapName);
}

void ADoomGameMode::HandleLevelCompletion()
{
	if (!DoomState)
	{
		return;
	}

	UE_LOG(LogDoomGameMode, Log, TEXT("Level completed: E%dM%d"),
		DoomState->GetGameEpisode(), DoomState->GetGameMap());

	bInLevel = false;

	// Transition to intermission state
	DoomState->SetGameState(EDoomGameState::Intermission);
	DoomState->SetGameAction(EDoomGameAction::Nothing);

	// Determine next map
	const EDoomGameMode GameMode = DoomState->GetGameMode();
	int32 NextEpisode = DoomState->GetGameEpisode();
	int32 NextMap = DoomState->GetGameMap() + 1;

	if (GameMode == EDoomGameMode::Commercial)
	{
		// DOOM 2: Maps 1-32, wrapping is handled by intermission
		if (NextMap > 32)
		{
			// Game complete - trigger finale
			DoomState->SetGameState(EDoomGameState::Finale);
			return;
		}
	}
	else
	{
		// DOOM 1: 9 maps per episode
		if (NextMap > 9)
		{
			// Episode complete
			NextMap = 1;
			NextEpisode++;

			int32 MaxEpisodes = 3; // Registered
			if (GameMode == EDoomGameMode::Retail)
			{
				MaxEpisodes = 4;
			}
			else if (GameMode == EDoomGameMode::Shareware)
			{
				MaxEpisodes = 1;
			}

			if (NextEpisode > MaxEpisodes)
			{
				// Game complete
				DoomState->SetGameState(EDoomGameState::Finale);
				return;
			}
		}
	}

	// Store next level info for when intermission completes
	DoomState->SetGameEpisode(NextEpisode);
	DoomState->SetGameMap(NextMap);
}

// =============================================================================
// Internal helpers
// =============================================================================

void ADoomGameMode::DetectGameMode()
{
	if (!WadManager || !DoomState)
	{
		return;
	}

	// Detection logic from d_main.c D_IdentifyVersion
	// Check for key lumps to determine game version

	const bool bHasE1M1 = WadManager->FindLump(TEXT("E1M1")) >= 0;
	const bool bHasE2M1 = WadManager->FindLump(TEXT("E2M1")) >= 0;
	const bool bHasE4M1 = WadManager->FindLump(TEXT("E4M1")) >= 0;
	const bool bHasMAP01 = WadManager->FindLump(TEXT("MAP01")) >= 0;

	if (bHasMAP01)
	{
		DoomState->SetGameMode(EDoomGameMode::Commercial);
		DoomState->SetGameMission(EDoomGameMission::Doom2);
		UE_LOG(LogDoomGameMode, Log, TEXT("Detected game mode: Commercial (DOOM 2)"));
	}
	else if (bHasE4M1)
	{
		DoomState->SetGameMode(EDoomGameMode::Retail);
		DoomState->SetGameMission(EDoomGameMission::Doom);
		UE_LOG(LogDoomGameMode, Log, TEXT("Detected game mode: Retail (Ultimate DOOM)"));
	}
	else if (bHasE2M1)
	{
		DoomState->SetGameMode(EDoomGameMode::Registered);
		DoomState->SetGameMission(EDoomGameMission::Doom);
		UE_LOG(LogDoomGameMode, Log, TEXT("Detected game mode: Registered (DOOM)"));
	}
	else if (bHasE1M1)
	{
		DoomState->SetGameMode(EDoomGameMode::Shareware);
		DoomState->SetGameMission(EDoomGameMission::Doom);
		UE_LOG(LogDoomGameMode, Log, TEXT("Detected game mode: Shareware (DOOM)"));
	}
	else
	{
		DoomState->SetGameMode(EDoomGameMode::Indetermined);
		DoomState->SetGameMission(EDoomGameMission::None);
		UE_LOG(LogDoomGameMode, Warning, TEXT("Could not determine game mode from WAD contents"));
	}
}

FString ADoomGameMode::BuildMapLumpName(int32 Episode, int32 Map) const
{
	if (DoomState && DoomState->GetGameMode() == EDoomGameMode::Commercial)
	{
		// DOOM 2 format: MAP01 - MAP32
		return FString::Printf(TEXT("MAP%02d"), Map);
	}
	else
	{
		// DOOM 1 format: E1M1 - E4M9
		return FString::Printf(TEXT("E%dM%d"), Episode, Map);
	}
}

void ADoomGameMode::CreateSubsystems()
{
	UE_LOG(LogDoomGameMode, Log, TEXT("Creating DOOM subsystems"));

	WadManager = NewObject<UWadFile>(this, TEXT("WadManager"));
	DoomState = NewObject<UDoomGameState>(this, TEXT("DoomGameState"));
	Specials = NewObject<UDoomSpecials>(this, TEXT("DoomSpecials"));

	// These subsystems are forward-declared and will be created when their
	// classes are implemented. For now, only create what exists.
	// GameLoop = NewObject<UDoomGameLoop>(this, TEXT("DoomGameLoop"));
	// StateMachine = NewObject<UDoomGameStateMachine>(this, TEXT("DoomStateMachine"));
	// ThinkerManager = NewObject<UDoomThinkerManager>(this, TEXT("DoomThinkerManager"));
	// AudioSystem = NewObject<UDoomAudioSystem>(this, TEXT("DoomAudioSystem"));
	// LevelLoader = NewObject<UDoomLevelLoader>(this, TEXT("DoomLevelLoader"));
	// BSPTree = NewObject<UDoomBSPTree>(this, TEXT("DoomBSPTree"));
}

void ADoomGameMode::DestroySubsystems()
{
	UE_LOG(LogDoomGameMode, Log, TEXT("Destroying DOOM subsystems"));

	if (WadManager)
	{
		WadManager->Close();
	}

	WadManager = nullptr;
	DoomState = nullptr;
	GameLoop = nullptr;
	StateMachine = nullptr;
	ThinkerManager = nullptr;
	AudioSystem = nullptr;
	LevelLoader = nullptr;
	BSPTree = nullptr;
	Specials = nullptr;

	bDoomInitialized = false;
}

void ADoomGameMode::RunSingleTic()
{
	if (!DoomState)
	{
		return;
	}

	// Increment the global tic counter
	DoomState->IncrementGameTic();

	// Check for pending game actions (equivalent to G_Ticker action processing)
	const EDoomGameAction Action = DoomState->GetGameAction();
	if (Action != EDoomGameAction::Nothing)
	{
		switch (Action)
		{
		case EDoomGameAction::LoadLevel:
			LoadLevel(DoomState->GetGameEpisode(), DoomState->GetGameMap());
			DoomState->SetGameAction(EDoomGameAction::Nothing);
			break;

		case EDoomGameAction::NewGame:
			StartNewGame(DoomState->GetGameSkill(), DoomState->GetGameEpisode(), DoomState->GetGameMap());
			DoomState->SetGameAction(EDoomGameAction::Nothing);
			break;

		case EDoomGameAction::Completed:
			HandleLevelCompletion();
			DoomState->SetGameAction(EDoomGameAction::Nothing);
			break;

		default:
			break;
		}
	}

	// Process based on current game state
	const EDoomGameState CurrentState = DoomState->GetGameState();
	switch (CurrentState)
	{
	case EDoomGameState::Level:
		if (bInLevel && !DoomState->IsPaused())
		{
			// Run thinkers (movers, monsters, projectiles, etc.)
			// if (ThinkerManager) { ThinkerManager->RunThinkers(); }

			// Update sector/line specials (doors, lifts, lights)
			if (Specials)
			{
				Specials->UpdateSpecials();
			}

			// Increment level time
			DoomState->IncrementLevelTime();
		}
		break;

	case EDoomGameState::Intermission:
		// Intermission screen update
		break;

	case EDoomGameState::Finale:
		// Finale screen update
		break;

	case EDoomGameState::DemoScreen:
		// Demo/title screen cycling
		break;
	}
}

void ADoomGameMode::SpawnLevelActor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Spawn the level geometry actor at the world origin
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = TEXT("DoomLevelGeometry");
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentLevelActor = World->SpawnActor<ADoomLevelActor>(
		ADoomLevelActor::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (CurrentLevelActor)
	{
		UE_LOG(LogDoomGameMode, Log, TEXT("Spawned DoomLevelActor"));
	}
}

void ADoomGameMode::CleanupCurrentLevel()
{
	if (CurrentLevelActor)
	{
		CurrentLevelActor->Destroy();
		CurrentLevelActor = nullptr;
	}

	bInLevel = false;
}
