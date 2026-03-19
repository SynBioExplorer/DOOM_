#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/DoomTypes.h"
#include "DoomGameState.generated.h"

/** Maximum number of players in a DOOM game. */
static constexpr int32 DOOM_MAXPLAYERS = 4;

/** DOOM tic rate: 35 tics per second. */
static constexpr int32 DOOM_TICRATE = 35;

/** Number of backup tics for networking. */
static constexpr int32 DOOM_BACKUPTICS = 12;

// EDoomGameMode, EDoomLanguage, EDoomGameState, EDoomSkill, EDoomGameAction
// are all defined in Core/DoomTypes.h

// EDoomMission (game mission) is defined in Core/DoomTypes.h

/**
 * UDoomGameState - Central repository for all global DOOM game state.
 *
 * Port of doomstat.h/doomstat.c global variables into a proper UObject
 * with encapsulated access. This replaces the scattered global variables
 * from the original DOOM source with a single authoritative state object.
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomGameState : public UObject
{
	GENERATED_BODY()

public:
	UDoomGameState();

	/** Initialize all state to default values. */
	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void Initialize();

	// -------------------------------------------
	// Game identification
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	EDoomGameMode GetGameMode() const { return GameMode; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameMode(EDoomGameMode InMode) { GameMode = InMode; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	EDoomMission GetGameMission() const { return GameMission; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameMission(EDoomMission InMission) { GameMission = InMission; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	EDoomLanguage GetLanguage() const { return Language; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetLanguage(EDoomLanguage InLanguage) { Language = InLanguage; }

	// -------------------------------------------
	// Game state machine
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	EDoomGameState GetGameState() const { return CurrentGameState; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameState(EDoomGameState InState) { CurrentGameState = InState; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	EDoomGameAction GetGameAction() const { return GameAction; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameAction(EDoomGameAction InAction) { GameAction = InAction; }

	// -------------------------------------------
	// Map / episode / skill
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	EDoomSkill GetGameSkill() const { return GameSkill; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameSkill(EDoomSkill InSkill) { GameSkill = InSkill; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetGameEpisode() const { return GameEpisode; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameEpisode(int32 InEpisode) { GameEpisode = InEpisode; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetGameMap() const { return GameMap; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameMap(int32 InMap) { GameMap = InMap; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool GetRespawnMonsters() const { return bRespawnMonsters; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetRespawnMonsters(bool bInRespawn) { bRespawnMonsters = bInRespawn; }

	// -------------------------------------------
	// Multiplayer / network flags
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsNetGame() const { return bNetGame; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetNetGame(bool bInNetGame) { bNetGame = bInNetGame; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsDeathmatch() const { return bDeathmatch; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetDeathmatch(bool bInDeathmatch) { bDeathmatch = bInDeathmatch; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetConsolePlayer() const { return ConsolePlayer; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetConsolePlayer(int32 InPlayer) { ConsolePlayer = InPlayer; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetDisplayPlayer() const { return DisplayPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetDisplayPlayer(int32 InPlayer) { DisplayPlayer = InPlayer; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsPlayerInGame(int32 PlayerIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetPlayerInGame(int32 PlayerIndex, bool bInGame);

	// -------------------------------------------
	// UI / pause flags
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsPaused() const { return bPaused; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetPaused(bool bInPaused) { bPaused = bInPaused; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsMenuActive() const { return bMenuActive; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetMenuActive(bool bInMenuActive) { bMenuActive = bInMenuActive; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsViewActive() const { return bViewActive; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetViewActive(bool bInViewActive) { bViewActive = bInViewActive; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsUserGame() const { return bUserGame; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetUserGame(bool bInUserGame) { bUserGame = bInUserGame; }

	// -------------------------------------------
	// Level statistics
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetLevelTime() const { return LevelTime; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetLevelTime(int32 InTime) { LevelTime = InTime; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void IncrementLevelTime() { LevelTime++; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetLevelStartTic() const { return LevelStartTic; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetLevelStartTic(int32 InTic) { LevelStartTic = InTic; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetGameTic() const { return GameTic; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetGameTic(int32 InTic) { GameTic = InTic; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void IncrementGameTic() { GameTic++; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetTotalKills() const { return TotalKills; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetTotalKills(int32 InKills) { TotalKills = InKills; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetTotalItems() const { return TotalItems; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetTotalItems(int32 InItems) { TotalItems = InItems; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetTotalSecrets() const { return TotalSecrets; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetTotalSecrets(int32 InSecrets) { TotalSecrets = InSecrets; }

	// -------------------------------------------
	// Demo state
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsDemoPlayback() const { return bDemoPlayback; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetDemoPlayback(bool bInPlayback) { bDemoPlayback = bInPlayback; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsDemoRecording() const { return bDemoRecording; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetDemoRecording(bool bInRecording) { bDemoRecording = bInRecording; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsSingleDemo() const { return bSingleDemo; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetSingleDemo(bool bInSingle) { bSingleDemo = bInSingle; }

	// -------------------------------------------
	// Command line parameters
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsDevParm() const { return bDevParm; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetDevParm(bool bInDevParm) { bDevParm = bInDevParm; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsNoMonsters() const { return bNoMonsters; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetNoMonsters(bool bInNoMonsters) { bNoMonsters = bInNoMonsters; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsRespawnParm() const { return bRespawnParm; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetRespawnParm(bool bInRespawn) { bRespawnParm = bInRespawn; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsFastParm() const { return bFastParm; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetFastParm(bool bInFast) { bFastParm = bInFast; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsModifiedGame() const { return bModifiedGame; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetModifiedGame(bool bInModified) { bModifiedGame = bInModified; }

	// -------------------------------------------
	// Timing / debug
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsPrecache() const { return bPrecache; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetPrecache(bool bInPrecache) { bPrecache = bInPrecache; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsTimingDemo() const { return bTimingDemo; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetTimingDemo(bool bInTiming) { bTimingDemo = bInTiming; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsNoDrawers() const { return bNoDrawers; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetNoDrawers(bool bInNoDrawers) { bNoDrawers = bInNoDrawers; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsNoBlit() const { return bNoBlit; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetNoBlit(bool bInNoBlit) { bNoBlit = bInNoBlit; }

	// -------------------------------------------
	// Network tic management
	// -------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetMakeTic() const { return MakeTic; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetMakeTic(int32 InTic) { MakeTic = InTic; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void IncrementMakeTic() { MakeTic++; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	int32 GetTicDup() const { return TicDup; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetTicDup(int32 InDup) { TicDup = InDup; }

	UFUNCTION(BlueprintPure, Category = "Doom|GameState")
	bool IsSingleTics() const { return bSingleTics; }

	UFUNCTION(BlueprintCallable, Category = "Doom|GameState")
	void SetSingleTics(bool bInSingle) { bSingleTics = bInSingle; }

protected:
	// -------------------------------------------
	// Game identification
	// -------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	EDoomGameMode GameMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	EDoomMission GameMission;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	EDoomLanguage Language;

	// -------------------------------------------
	// Game state machine
	// -------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	EDoomGameState CurrentGameState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	EDoomGameAction GameAction;

	// -------------------------------------------
	// Map / episode / skill selection
	// -------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	EDoomSkill GameSkill;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 GameEpisode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 GameMap;

	/** Nightmare mode flag - monsters respawn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bRespawnMonsters;

	// -------------------------------------------
	// Player state
	// -------------------------------------------

	/** Which players are currently in the game. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	TArray<bool> PlayerInGame;

	/** Player taking events and displaying (local player). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 ConsolePlayer;

	/** Player whose view is being displayed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 DisplayPlayer;

	// -------------------------------------------
	// Multiplayer / network flags
	// -------------------------------------------

	/** True only if >1 player (packets are broadcast). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bNetGame;

	/** True if started as net deathmatch. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bDeathmatch;

	// -------------------------------------------
	// UI / pause state
	// -------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bPaused;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bMenuActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bViewActive;

	/** True if user can save/end game (false during demos). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bUserGame;

	// -------------------------------------------
	// Level statistics / timing
	// -------------------------------------------

	/** Global game tic counter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 GameTic;

	/** Gametic at level start. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 LevelStartTic;

	/** Tics played in the current level (for par time). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 LevelTime;

	/** Intermission statistics. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 TotalKills;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 TotalItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 TotalSecrets;

	// -------------------------------------------
	// Demo recording / playback
	// -------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bDemoPlayback;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bDemoRecording;

	/** Quit after playing a demo from command line. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bSingleDemo;

	// -------------------------------------------
	// Command line parameters
	// -------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bDevParm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bNoMonsters;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bRespawnParm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bFastParm;

	/** Set if homebrew PWAD stuff has been added. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bModifiedGame;

	// -------------------------------------------
	// Rendering / timing
	// -------------------------------------------

	/** If true, load all graphics at level load. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bPrecache;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bTimingDemo;

	/** For comparative timing purposes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bNoDrawers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bNoBlit;

	// -------------------------------------------
	// Network tic management
	// -------------------------------------------

	/** The tic that hasn't had control made for it yet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 MakeTic;

	/** Tic duplication factor for slow networks. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	int32 TicDup;

	/** Debug flag to cancel adaptiveness. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|GameState")
	bool bSingleTics;
};
