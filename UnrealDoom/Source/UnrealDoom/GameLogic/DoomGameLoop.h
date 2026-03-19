#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoomTicCmd.h"
#include "DoomGameLoop.generated.h"

class UDoomGameState;
class UDoomThinkerManager;

/**
 * UDoomGameLoop - Port of D_DoomLoop logic adapted to UE5's tick system.
 *
 * The original DOOM runs D_DoomLoop() as an infinite while(1) loop that
 * calls I_StartFrame, processes events, builds tic commands, runs game
 * tics, and renders. In UE5, we cannot spin in an infinite loop. Instead,
 * this class provides a Tick() method designed to be called each frame by
 * the owning game mode or subsystem. It uses an accumulator-based fixed
 * timestep to maintain DOOM's 35 tics/second rate.
 *
 * Key adaptations from D_DoomLoop / TryRunTics / NetUpdate:
 * - Frame timing uses UE5's FApp::GetDeltaTime() instead of I_GetTime()
 * - Fixed timestep accumulator replaces the original tic counting
 * - Input processing delegates to UE5's input system
 * - Network tic synchronization is stubbed for future implementation
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomGameLoop : public UObject
{
	GENERATED_BODY()

public:
	UDoomGameLoop();

	/**
	 * Initialize the game loop with required subsystem references.
	 * Must be called before Tick().
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|GameLoop")
	void Initialize(UDoomGameState* InGameState, UDoomThinkerManager* InThinkerManager);

	/**
	 * Main tick function - call this every frame from your game mode or subsystem.
	 * Handles accumulator-based fixed timestep to maintain 35 tics/sec.
	 *
	 * This replaces the original D_DoomLoop's while(1) pattern:
	 *   I_StartFrame();
	 *   if (singletics) { process one tic; }
	 *   else { TryRunTics(); }
	 *   D_Display();
	 *
	 * @param DeltaTime Frame delta time in seconds (from UE5's tick).
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|GameLoop")
	void Tick(float DeltaTime);

	/**
	 * Process pending input events through the responder chain.
	 * Port of D_ProcessEvents() from d_main.c.
	 *
	 * In the original: events are queued by D_PostEvent and consumed here,
	 * passing through M_Responder (menu) then G_Responder (game).
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|GameLoop")
	void ProcessEvents();

	/**
	 * Build a tic command from current input state.
	 * Port of G_BuildTiccmd() from g_game.c.
	 *
	 * Samples keyboard, mouse, and joystick state to produce a
	 * ticcmd_t for the current tic.
	 *
	 * @param OutCmd The tic command to fill in.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|GameLoop")
	void BuildTicCmd(FDoomTicCmd& OutCmd);

	/**
	 * Run all ticker functions for a single game tic.
	 * Port of G_Ticker() dispatch - calls the appropriate subsystem
	 * tickers based on current game state.
	 *
	 * Original G_Ticker does:
	 * 1. Player reborns
	 * 2. Process game actions (state transitions)
	 * 3. Copy net commands to players
	 * 4. Check special buttons (pause, save)
	 * 5. Call state-specific ticker (P_Ticker, WI_Ticker, F_Ticker, D_PageTicker)
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|GameLoop")
	void RunTickers();

	/**
	 * Timing and tic synchronization.
	 * Simplified port of TryRunTics() from d_net.c.
	 *
	 * The original TryRunTics handles network synchronization, tic counting,
	 * and decides how many tics to run. In this single-player focused port,
	 * it runs tics based on the accumulated time.
	 *
	 * @param TicsToRun Number of tics to execute this frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|GameLoop")
	void TryRunTics(int32 TicsToRun);

	/** Get the current tic command buffer for a player. */
	const FDoomTicCmd& GetNetCmd(int32 PlayerIndex, int32 TicIndex) const;

	/** Set a tic command in the buffer. */
	void SetNetCmd(int32 PlayerIndex, int32 TicIndex, const FDoomTicCmd& Cmd);

	/** Delegate fired when a game tic is about to execute. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameTic, int32, TicNumber);

	UPROPERTY(BlueprintAssignable, Category = "Doom|GameLoop")
	FOnGameTic OnPreGameTic;

	UPROPERTY(BlueprintAssignable, Category = "Doom|GameLoop")
	FOnGameTic OnPostGameTic;

	/** Get the time accumulator value (for debugging). */
	UFUNCTION(BlueprintPure, Category = "Doom|GameLoop")
	float GetTimeAccumulator() const { return TimeAccumulator; }

	/** Check if the game loop has been initialized. */
	UFUNCTION(BlueprintPure, Category = "Doom|GameLoop")
	bool IsInitialized() const { return GameState != nullptr && ThinkerManager != nullptr; }

protected:
	/**
	 * Run a single game tic. Called by TryRunTics for each tic to execute.
	 * This is the inner loop body equivalent to what happens inside
	 * TryRunTics' "while (counts--)" loop.
	 */
	void ExecuteSingleTic();

	/**
	 * Handle demo advance between tics.
	 * Port of D_DoAdvanceDemo() logic.
	 */
	void HandleAdvanceDemo();

private:
	/** Reference to the global game state. */
	UPROPERTY()
	TObjectPtr<UDoomGameState> GameState;

	/** Reference to the thinker manager. */
	UPROPERTY()
	TObjectPtr<UDoomThinkerManager> ThinkerManager;

	/**
	 * Time accumulator for fixed timestep.
	 * Accumulates real time (seconds) and drains by TicDuration per tic executed.
	 * This replaces the original I_GetTime() based tic counting.
	 */
	float TimeAccumulator;

	/** Duration of a single tic in seconds (1.0 / TICRATE = ~0.02857). */
	static constexpr float TicDuration = 1.0f / static_cast<float>(DOOM_TICRATE);

	/**
	 * Maximum tics to process per frame to prevent spiral of death.
	 * If the game falls behind, we cap the catch-up to avoid freezing.
	 */
	static constexpr int32 MaxTicsPerFrame = 4;

	/**
	 * Network command buffer. Stores tic commands for all players.
	 * NetCmds[PlayerIndex][TicIndex % BACKUPTICS]
	 * Port of netcmds[MAXPLAYERS][BACKUPTICS] from d_net.c.
	 */
	FDoomTicCmd NetCmds[DOOM_MAXPLAYERS][DOOM_BACKUPTICS];

	/** Flag to advance the demo sequence (port of advancedemo). */
	bool bAdvanceDemo;
};
