#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// ADoomGameMode - Master game mode that initializes and drives
// the DOOM engine loop within the Unreal Engine framework.
// Equivalent to d_main.c / d_net.c / g_game.c coordination.

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DoomTypes.h"
#include "DoomGameMode.generated.h"

class UWadFile;
class UDoomGameState;
class ADoomPlayerPawn;
class ADoomPlayerController;
class ADoomLevelActor;

// Forward declarations for subsystems (to be implemented)
class UDoomGameLoop;
class UDoomGameStateMachine;
class UDoomThinkerManager;
class UDoomAudioSystem;
class UDoomLevelLoader;
class UDoomBSPTree;
class UDoomSpecials;

/**
 * ADoomGameMode - The central game mode that ties together all DOOM subsystems.
 *
 * Responsibilities:
 * - Initializes the WAD file system and detects the game version
 * - Creates and owns all DOOM subsystem objects
 * - Runs the DOOM game loop at the original 35 tics/sec rate
 * - Manages level loading, transitions, and game flow
 *
 * This replaces d_main.c's D_DoomMain and the main loop from d_net.c.
 */
UCLASS(Blueprintable, ClassGroup = "Doom")
class UNREALDOOM_API ADoomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADoomGameMode();

	// =========================================================================
	// AGameModeBase overrides
	// =========================================================================

	/** Called before any other initialization. Sets up DOOM subsystem objects. */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** Called when the game actually starts playing. Begins the DOOM game loop. */
	virtual void StartPlay() override;

	/** Main tick - runs the DOOM game loop at 35 tics/sec. */
	virtual void Tick(float DeltaSeconds) override;

	// =========================================================================
	// DOOM initialization
	// =========================================================================

	/**
	 * Initialize the DOOM engine by loading a WAD file.
	 * Detects game mode (shareware, registered, commercial, retail),
	 * and initializes all subsystems.
	 * Equivalent to D_DoomMain.
	 *
	 * @param WadPath - Absolute path to the IWAD file
	 * @return true if initialization succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Init")
	bool InitializeDoom(const FString& WadPath);

	// =========================================================================
	// Game flow
	// =========================================================================

	/**
	 * Start a new game with the given parameters.
	 * Equivalent to G_DeferedInitNew / G_DoNewGame / G_InitNew.
	 *
	 * @param Skill - Difficulty level
	 * @param Episode - Episode number (1-4 for DOOM1, 1 for DOOM2)
	 * @param Map - Map number within the episode
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Game")
	void StartNewGame(EDoomSkill Skill, int32 Episode, int32 Map);

	/**
	 * Load and set up a specific level.
	 * Parses map data from the WAD, generates geometry,
	 * spawns things, and initializes specials.
	 * Equivalent to P_SetupLevel.
	 *
	 * @param Episode - Episode number
	 * @param Map - Map number
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Game")
	void LoadLevel(int32 Episode, int32 Map);

	/**
	 * Called when the current level is completed.
	 * Triggers intermission screen or next level transition.
	 * Equivalent to G_ExitLevel / G_SecretExitLevel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Game")
	void HandleLevelCompletion();

	// =========================================================================
	// Subsystem access
	// =========================================================================

	/** @return The WAD file manager */
	UFUNCTION(BlueprintPure, Category = "Doom|Subsystems")
	UWadFile* GetWadManager() const { return WadManager; }

	/** @return The global game state */
	UFUNCTION(BlueprintPure, Category = "Doom|Subsystems")
	UDoomGameState* GetDoomGameState() const { return DoomState; }

	/** @return The thinker manager */
	UFUNCTION(BlueprintPure, Category = "Doom|Subsystems")
	UDoomThinkerManager* GetThinkerManager() const { return ThinkerManager; }

	/** @return The audio system */
	UFUNCTION(BlueprintPure, Category = "Doom|Subsystems")
	UDoomAudioSystem* GetAudioSystem() const { return AudioSystem; }

	/** @return The level loader */
	UFUNCTION(BlueprintPure, Category = "Doom|Subsystems")
	UDoomLevelLoader* GetLevelLoader() const { return LevelLoader; }

	/** @return The BSP tree */
	UFUNCTION(BlueprintPure, Category = "Doom|Subsystems")
	UDoomBSPTree* GetBSPTree() const { return BSPTree; }

	/** @return The specials manager */
	UFUNCTION(BlueprintPure, Category = "Doom|Subsystems")
	UDoomSpecials* GetSpecials() const { return Specials; }

	/** @return Whether the DOOM engine has been initialized */
	UFUNCTION(BlueprintPure, Category = "Doom|Init")
	bool IsDoomInitialized() const { return bDoomInitialized; }

	/** @return The current level actor holding generated geometry */
	UFUNCTION(BlueprintPure, Category = "Doom|Game")
	ADoomLevelActor* GetLevelActor() const { return CurrentLevelActor; }

protected:
	// =========================================================================
	// Internal helpers
	// =========================================================================

	/** Detect the game mode from the loaded WAD contents. */
	void DetectGameMode();

	/** Build the map lump name string (e.g., "E1M1" or "MAP01"). */
	FString BuildMapLumpName(int32 Episode, int32 Map) const;

	/** Create all subsystem UObjects. Called during InitGame. */
	void CreateSubsystems();

	/** Destroy all subsystem UObjects. */
	void DestroySubsystems();

	/** Run a single DOOM game tic. Equivalent to TryRunTics / G_Ticker. */
	void RunSingleTic();

	/** Spawn the level actor into the world. */
	void SpawnLevelActor();

	/** Destroy the current level actor and clean up level state. */
	void CleanupCurrentLevel();

	// =========================================================================
	// Subsystem references
	// =========================================================================

	/** WAD file manager - handles reading lumps from the IWAD/PWAD */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UWadFile> WadManager;

	/** Global DOOM game state (skill, episode, map, flags, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomGameState> DoomState;

	/** Main game loop coordinator */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomGameLoop> GameLoop;

	/** Game state machine (menus, level, intermission, finale) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomGameStateMachine> StateMachine;

	/** Thinker list manager - runs all active thinkers each tic */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomThinkerManager> ThinkerManager;

	/** Audio system - SFX and music playback */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomAudioSystem> AudioSystem;

	/** Level data loader - parses WAD map lumps into runtime structures */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomLevelLoader> LevelLoader;

	/** BSP tree for the current level */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomBSPTree> BSPTree;

	/** Sector/line specials manager (doors, lifts, crushers, lights, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Subsystems")
	TObjectPtr<UDoomSpecials> Specials;

	// =========================================================================
	// Level state
	// =========================================================================

	/** The actor that holds all generated level geometry */
	UPROPERTY(Transient)
	TObjectPtr<ADoomLevelActor> CurrentLevelActor;

	// =========================================================================
	// Configuration
	// =========================================================================

	/** Path to the WAD file. Can be set from Blueprint or command line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Config")
	FString WadFilePath;

	/** HUD widget class to spawn for the DOOM HUD overlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Config")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	// =========================================================================
	// Timing
	// =========================================================================

	/** Accumulated real time for tic scheduling */
	float AccumulatedTime = 0.0f;

	/** Seconds per DOOM tic (1.0 / 35.0) */
	static constexpr float SecondsPerTic = 1.0f / static_cast<float>(TICRATE);

	/** Whether the DOOM engine has been fully initialized */
	bool bDoomInitialized = false;

	/** Whether we are currently in a level */
	bool bInLevel = false;
};
