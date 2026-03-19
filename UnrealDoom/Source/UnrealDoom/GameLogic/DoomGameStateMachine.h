#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoomGameState.h"
#include "DoomGameStateMachine.generated.h"

class UDoomGameLoop;
class UDoomThinkerManager;

/**
 * UDoomGameStateMachine - Port of the G_Ticker state machine from g_game.c.
 *
 * Handles game state transitions (game actions) that were processed in the
 * main G_Ticker() function's action dispatch loop. In the original DOOM,
 * game actions are deferred requests that get processed at the start of
 * each tic. This class centralizes that logic.
 *
 * Game States (what screen/mode the game is in):
 *   Level, Intermission, Finale, DemoScreen
 *
 * Game Actions (deferred state transitions):
 *   LoadLevel, NewGame, LoadGame, SaveGame, Completed, Victory, WorldDone, etc.
 *
 * Original flow in G_Ticker():
 *   while (gameaction != ga_nothing) {
 *       switch (gameaction) {
 *           case ga_loadlevel: G_DoLoadLevel(); break;
 *           case ga_newgame:   G_DoNewGame(); break;
 *           ...
 *       }
 *   }
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomGameStateMachine : public UObject
{
	GENERATED_BODY()

public:
	UDoomGameStateMachine();

	/**
	 * Initialize with required subsystem references.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void Initialize(UDoomGameState* InGameState, UDoomGameLoop* InGameLoop, UDoomThinkerManager* InThinkerManager);

	/**
	 * Process any pending game action. Called at the start of each game tic.
	 * Port of the gameaction dispatch loop in G_Ticker().
	 *
	 * Original: while (gameaction != ga_nothing) { switch (gameaction) { ... } }
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void ProcessGameActions();

	// -------------------------------------------
	// Game action requests (set gameaction for deferred processing)
	// -------------------------------------------

	/**
	 * Request a new game. Port of G_DeferedInitNew().
	 * Sets gameaction = ga_newgame and stores the parameters.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestNewGame(EDoomSkill Skill, int32 Episode, int32 Map);

	/**
	 * Request level load. Sets gameaction = ga_loadlevel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestLoadLevel();

	/**
	 * Request game load from a save slot. Port of G_LoadGame().
	 * @param SaveName Path/name of the save file.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestLoadGame(const FString& SaveName);

	/**
	 * Request game save to a slot. Port of G_SaveGame().
	 * @param Slot Save slot number.
	 * @param Description Save description string.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestSaveGame(int32 Slot, const FString& Description);

	/**
	 * Request level completion. Port of G_ExitLevel().
	 * @param bSecretExit True if exiting via secret exit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestExitLevel(bool bSecretExit = false);

	/**
	 * Request demo playback. Port of G_DeferedPlayDemo().
	 * @param DemoName Name of the demo lump.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestPlayDemo(const FString& DemoName);

	/**
	 * Request screenshot. Sets gameaction = ga_screenshot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestScreenshot();

	/**
	 * Notify that the world/intermission is done, proceed to next level.
	 * Port of G_WorldDone().
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void RequestWorldDone();

	// -------------------------------------------
	// Delegates for state transitions
	// -------------------------------------------

	/** Fired when a new level is about to load. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelLoad, int32, Episode, int32, Map);
	UPROPERTY(BlueprintAssignable, Category = "Doom|StateMachine")
	FOnLevelLoad OnLevelLoad;

	/** Fired when a new game is starting. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNewGame, EDoomSkill, Skill, int32, Episode, int32, Map);
	UPROPERTY(BlueprintAssignable, Category = "Doom|StateMachine")
	FOnNewGame OnNewGame;

	/** Fired when a level is completed. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelCompleted, bool, bSecretExit);
	UPROPERTY(BlueprintAssignable, Category = "Doom|StateMachine")
	FOnLevelCompleted OnLevelCompleted;

	/** Fired when the game state changes. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameStateChanged, EDoomGameState, OldState, EDoomGameState, NewState);
	UPROPERTY(BlueprintAssignable, Category = "Doom|StateMachine")
	FOnGameStateChanged OnGameStateChanged;

	// -------------------------------------------
	// Demo recording/playback stubs
	// -------------------------------------------

	/**
	 * Begin recording a demo. Stub for G_RecordDemo() / G_BeginRecording().
	 * @param DemoName Name for the demo file.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void BeginDemoRecording(const FString& DemoName);

	/**
	 * Stop recording the current demo. Stub for G_CheckDemoStatus() recording path.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	void StopDemoRecording();

	/**
	 * Check demo status - called after death or level completion.
	 * Port of G_CheckDemoStatus().
	 * @return True if a new demo loop action will take place.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|StateMachine")
	bool CheckDemoStatus();

protected:
	// -------------------------------------------
	// Action handlers (G_Do* functions)
	// -------------------------------------------

	/**
	 * Execute level load. Port of G_DoLoadLevel() from g_game.c.
	 *
	 * Original:
	 * - Sets sky texture based on episode/map
	 * - Sets levelstarttic = gametic
	 * - Sets gamestate = GS_LEVEL
	 * - Reborns dead players, clears frags
	 * - Calls P_SetupLevel()
	 * - Clears input state
	 */
	void DoLoadLevel();

	/**
	 * Execute new game. Port of G_DoNewGame() from g_game.c.
	 *
	 * Original:
	 * - Resets demo/net state
	 * - Clears extra players
	 * - Resets parameters
	 * - Calls G_InitNew()
	 */
	void DoNewGame();

	/**
	 * Initialize a new game with specific parameters. Port of G_InitNew().
	 *
	 * Original:
	 * - Clamp skill/episode/map
	 * - Set nightmare respawn behavior
	 * - Set fast monsters if nightmare
	 * - Force player reborn
	 * - Set sky texture for episode
	 * - Call G_DoLoadLevel()
	 */
	void InitNewGame(EDoomSkill Skill, int32 Episode, int32 Map);

	/**
	 * Execute game load. Port of G_DoLoadGame() from g_game.c.
	 * Stub - actual save/load implementation deferred.
	 */
	void DoLoadGame();

	/**
	 * Execute game save. Port of G_DoSaveGame() from g_game.c.
	 * Stub - actual save/load implementation deferred.
	 */
	void DoSaveGame();

	/**
	 * Execute level completion. Port of G_DoCompleted() from g_game.c.
	 *
	 * Original:
	 * - Finishes players (clear powers, cards)
	 * - Checks for episode end (victory)
	 * - Calculates next map (handling secret exits)
	 * - Fills in wminfo for intermission
	 * - Sets gamestate = GS_INTERMISSION
	 * - Starts intermission screen
	 */
	void DoCompleted();

	/**
	 * Execute world done (intermission finished). Port of G_DoWorldDone().
	 *
	 * Original:
	 * - Sets gamestate = GS_LEVEL
	 * - Sets gamemap = wminfo.next + 1
	 * - Calls G_DoLoadLevel()
	 */
	void DoWorldDone();

	/**
	 * Execute demo playback start. Port of G_DoPlayDemo().
	 * Stub - demo system implementation deferred.
	 */
	void DoPlayDemo();

	/**
	 * Execute victory. Starts the finale sequence.
	 * Original: F_StartFinale();
	 */
	void DoVictory();

	/**
	 * Calculate the next map after completing the current one.
	 * Handles secret exits and DOOM 1 vs DOOM 2 map progression.
	 * @param bSecretExit Whether the player used a secret exit.
	 * @return The next map number (0-based for wminfo.next).
	 */
	int32 CalculateNextMap(bool bSecretExit) const;

private:
	/** Reference to the global game state. */
	UPROPERTY()
	TObjectPtr<UDoomGameState> GameState;

	/** Reference to the game loop. */
	UPROPERTY()
	TObjectPtr<UDoomGameLoop> GameLoop;

	/** Reference to the thinker manager. */
	UPROPERTY()
	TObjectPtr<UDoomThinkerManager> ThinkerManager;

	// -------------------------------------------
	// Deferred action parameters
	// (equivalent to the d_skill, d_episode, d_map globals)
	// -------------------------------------------

	/** Deferred skill for new game. */
	EDoomSkill DeferredSkill;

	/** Deferred episode for new game. */
	int32 DeferredEpisode;

	/** Deferred map for new game. */
	int32 DeferredMap;

	/** Save game slot for deferred save. */
	int32 SaveGameSlot;

	/** Save game description for deferred save. */
	FString SaveDescription;

	/** Save file name for deferred load. */
	FString SaveFileName;

	/** Demo name for deferred playback. */
	FString DeferredDemoName;

	/** Whether the current level exit is a secret exit. */
	bool bSecretExit;

	/** Next map to load after intermission (0-based, like wminfo.next). */
	int32 NextMap;

	/** Name of the demo currently being recorded. */
	FString RecordingDemoName;
};
