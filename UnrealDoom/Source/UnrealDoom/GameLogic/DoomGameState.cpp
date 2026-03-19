#include "DoomGameState.h"

UDoomGameState::UDoomGameState()
{
	Initialize();
}

void UDoomGameState::Initialize()
{
	// Game identification
	GameMode = EDoomGameMode::Indetermined;
	GameMission = EDoomGameMission::Doom;
	Language = EDoomLanguage::English;

	// Game state machine
	CurrentGameState = EDoomGameState::DemoScreen;
	GameAction = EDoomGameAction::Nothing;

	// Map / episode / skill
	GameSkill = EDoomSkill::Medium;
	GameEpisode = 1;
	GameMap = 1;
	bRespawnMonsters = false;

	// Player state
	PlayerInGame.Init(false, DOOM_MAXPLAYERS);
	ConsolePlayer = 0;
	DisplayPlayer = 0;

	// Multiplayer / network
	bNetGame = false;
	bDeathmatch = false;

	// UI / pause
	bPaused = false;
	bMenuActive = false;
	bViewActive = true;
	bUserGame = false;

	// Level statistics
	GameTic = 0;
	LevelStartTic = 0;
	LevelTime = 0;
	TotalKills = 0;
	TotalItems = 0;
	TotalSecrets = 0;

	// Demo state
	bDemoPlayback = false;
	bDemoRecording = false;
	bSingleDemo = false;

	// Command line parameters
	bDevParm = false;
	bNoMonsters = false;
	bRespawnParm = false;
	bFastParm = false;
	bModifiedGame = false;

	// Rendering / timing
	bPrecache = true;
	bTimingDemo = false;
	bNoDrawers = false;
	bNoBlit = false;

	// Network tic management
	MakeTic = 0;
	TicDup = 1;
	bSingleTics = false;
}

bool UDoomGameState::IsPlayerInGame(int32 PlayerIndex) const
{
	if (PlayerIndex >= 0 && PlayerIndex < PlayerInGame.Num())
	{
		return PlayerInGame[PlayerIndex];
	}
	return false;
}

void UDoomGameState::SetPlayerInGame(int32 PlayerIndex, bool bInGame)
{
	if (PlayerIndex >= 0 && PlayerIndex < PlayerInGame.Num())
	{
		PlayerInGame[PlayerIndex] = bInGame;
	}
}
