#include "DoomGameStateMachine.h"
#include "DoomGameLoop.h"
#include "DoomThinker.h"

UDoomGameStateMachine::UDoomGameStateMachine()
	: DeferredSkill(EDoomSkill::Medium)
	, DeferredEpisode(1)
	, DeferredMap(1)
	, SaveGameSlot(0)
	, bSecretExit(false)
	, NextMap(0)
{
}

void UDoomGameStateMachine::Initialize(UDoomGameState* InGameState, UDoomGameLoop* InGameLoop, UDoomThinkerManager* InThinkerManager)
{
	GameState = InGameState;
	GameLoop = InGameLoop;
	ThinkerManager = InThinkerManager;
}

void UDoomGameStateMachine::ProcessGameActions()
{
	// Port of the game action dispatch loop in G_Ticker():
	//   while (gameaction != ga_nothing) {
	//       switch (gameaction) { ... }
	//   }

	if (!GameState)
	{
		return;
	}

	while (GameState->GetGameAction() != EDoomGameAction::Nothing)
	{
		switch (GameState->GetGameAction())
		{
		case EDoomGameAction::LoadLevel:
			DoLoadLevel();
			break;

		case EDoomGameAction::NewGame:
			DoNewGame();
			break;

		case EDoomGameAction::LoadGame:
			DoLoadGame();
			break;

		case EDoomGameAction::SaveGame:
			DoSaveGame();
			break;

		case EDoomGameAction::PlayDemo:
			DoPlayDemo();
			break;

		case EDoomGameAction::Completed:
			DoCompleted();
			break;

		case EDoomGameAction::Victory:
			DoVictory();
			break;

		case EDoomGameAction::WorldDone:
			DoWorldDone();
			break;

		case EDoomGameAction::Screenshot:
			// Original: M_ScreenShot(); gameaction = ga_nothing;
			// TODO: Implement screenshot via UE5's screenshot system.
			UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Screenshot requested"));
			GameState->SetGameAction(EDoomGameAction::Nothing);
			break;

		case EDoomGameAction::Nothing:
			break;
		}
	}
}

// -------------------------------------------
// Game action requests
// -------------------------------------------

void UDoomGameStateMachine::RequestNewGame(EDoomSkill Skill, int32 Episode, int32 Map)
{
	// Port of G_DeferedInitNew():
	//   d_skill = skill; d_episode = episode; d_map = map;
	//   gameaction = ga_newgame;
	DeferredSkill = Skill;
	DeferredEpisode = Episode;
	DeferredMap = Map;

	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::NewGame);
	}
}

void UDoomGameStateMachine::RequestLoadLevel()
{
	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::LoadLevel);
	}
}

void UDoomGameStateMachine::RequestLoadGame(const FString& SaveName)
{
	// Port of G_LoadGame():
	//   strcpy(savename, name); gameaction = ga_loadgame;
	SaveFileName = SaveName;

	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::LoadGame);
	}
}

void UDoomGameStateMachine::RequestSaveGame(int32 Slot, const FString& Description)
{
	// Port of G_SaveGame():
	//   savegameslot = slot; strcpy(savedescription, description); sendsave = true;
	SaveGameSlot = Slot;
	SaveDescription = Description;

	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::SaveGame);
	}
}

void UDoomGameStateMachine::RequestExitLevel(bool bInSecretExit)
{
	// Port of G_ExitLevel() / G_SecretExitLevel():
	//   secretexit = false/true; gameaction = ga_completed;
	bSecretExit = bInSecretExit;

	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::Completed);
	}
}

void UDoomGameStateMachine::RequestPlayDemo(const FString& DemoName)
{
	// Port of G_DeferedPlayDemo():
	//   defdemoname = name; gameaction = ga_playdemo;
	DeferredDemoName = DemoName;

	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::PlayDemo);
	}
}

void UDoomGameStateMachine::RequestScreenshot()
{
	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::Screenshot);
	}
}

void UDoomGameStateMachine::RequestWorldDone()
{
	// Port of G_WorldDone():
	//   gameaction = ga_worlddone;
	//   if (secretexit) players[consoleplayer].didsecret = true;

	if (GameState)
	{
		GameState->SetGameAction(EDoomGameAction::WorldDone);

		// In the original, certain DOOM 2 maps trigger a finale on completion.
		// Original checks gamemap 6, 11, 15, 20, 30, 31 for finale triggers.
		// TODO: Trigger F_StartFinale() for specific DOOM 2 maps.
	}
}

// -------------------------------------------
// Action handlers
// -------------------------------------------

void UDoomGameStateMachine::DoLoadLevel()
{
	// Port of G_DoLoadLevel() from g_game.c.
	//
	// Original:
	//   levelstarttic = gametic;
	//   gamestate = GS_LEVEL;
	//   for (i=0; i<MAXPLAYERS; i++) {
	//       if (playeringame[i] && players[i].playerstate == PST_DEAD)
	//           players[i].playerstate = PST_REBORN;
	//       memset(players[i].frags, 0, sizeof(players[i].frags));
	//   }
	//   P_SetupLevel(gameepisode, gamemap, 0, gameskill);
	//   displayplayer = consoleplayer;
	//   gameaction = ga_nothing;

	if (!GameState)
	{
		return;
	}

	const EDoomGameState OldState = GameState->GetGameState();

	GameState->SetLevelStartTic(GameState->GetGameTic());
	GameState->SetGameState(EDoomGameState::Level);
	GameState->SetDisplayPlayer(GameState->GetConsolePlayer());
	GameState->SetPaused(false);
	GameState->SetGameAction(EDoomGameAction::Nothing);

	// Reset level statistics.
	GameState->SetLevelTime(0);
	GameState->SetTotalKills(0);
	GameState->SetTotalItems(0);
	GameState->SetTotalSecrets(0);

	// Initialize thinker list for the new level.
	if (ThinkerManager)
	{
		ThinkerManager->InitThinkers();
	}

	// Fire delegates.
	OnLevelLoad.Broadcast(GameState->GetGameEpisode(), GameState->GetGameMap());

	if (OldState != EDoomGameState::Level)
	{
		OnGameStateChanged.Broadcast(OldState, EDoomGameState::Level);
	}

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Loading E%dM%d (Skill: %d)"),
		GameState->GetGameEpisode(), GameState->GetGameMap(),
		static_cast<int32>(GameState->GetGameSkill()));
}

void UDoomGameStateMachine::DoNewGame()
{
	// Port of G_DoNewGame() from g_game.c.
	//
	// Original:
	//   demoplayback = false; netdemo = false; netgame = false;
	//   deathmatch = false;
	//   playeringame[1] = playeringame[2] = playeringame[3] = 0;
	//   respawnparm = false; fastparm = false; nomonsters = false;
	//   consoleplayer = 0;
	//   G_InitNew(d_skill, d_episode, d_map);
	//   gameaction = ga_nothing;

	if (!GameState)
	{
		return;
	}

	GameState->SetDemoPlayback(false);
	GameState->SetNetGame(false);
	GameState->SetDeathmatch(false);
	GameState->SetRespawnParm(false);
	GameState->SetFastParm(false);
	GameState->SetNoMonsters(false);
	GameState->SetConsolePlayer(0);

	// Clear extra players.
	for (int32 i = 1; i < DOOM_MAXPLAYERS; ++i)
	{
		GameState->SetPlayerInGame(i, false);
	}

	InitNewGame(DeferredSkill, DeferredEpisode, DeferredMap);
	GameState->SetGameAction(EDoomGameAction::Nothing);

	OnNewGame.Broadcast(DeferredSkill, DeferredEpisode, DeferredMap);
}

void UDoomGameStateMachine::InitNewGame(EDoomSkill Skill, int32 Episode, int32 Map)
{
	// Port of G_InitNew() from g_game.c.
	//
	// Original:
	//   Clamp skill to sk_nightmare max
	//   Clamp episode based on game mode
	//   Clamp map (1-9 for non-commercial)
	//   Set respawnmonsters for nightmare
	//   Set fast monsters for nightmare
	//   Force all players to PST_REBORN
	//   Set usergame=true, paused=false, demoplayback=false
	//   Set gameepisode, gamemap, gameskill
	//   Call G_DoLoadLevel()

	if (!GameState)
	{
		return;
	}

	// Unpause if paused.
	if (GameState->IsPaused())
	{
		GameState->SetPaused(false);
	}

	// Clamp skill.
	if (Skill > EDoomSkill::Nightmare)
	{
		Skill = EDoomSkill::Nightmare;
	}

	// Clamp episode based on game mode.
	if (Episode < 1)
	{
		Episode = 1;
	}

	switch (GameState->GetGameMode())
	{
	case EDoomGameMode::Retail:
		if (Episode > 4) Episode = 4;
		break;
	case EDoomGameMode::Shareware:
		if (Episode > 1) Episode = 1;
		break;
	default:
		if (Episode > 3) Episode = 3;
		break;
	}

	// Clamp map.
	if (Map < 1)
	{
		Map = 1;
	}
	if (Map > 9 && GameState->GetGameMode() != EDoomGameMode::Commercial)
	{
		Map = 9;
	}

	// Set respawn monsters for nightmare.
	if (Skill == EDoomSkill::Nightmare || GameState->IsRespawnParm())
	{
		GameState->SetRespawnMonsters(true);
	}
	else
	{
		GameState->SetRespawnMonsters(false);
	}

	// Set game parameters.
	GameState->SetUserGame(true);
	GameState->SetPaused(false);
	GameState->SetDemoPlayback(false);
	GameState->SetViewActive(true);
	GameState->SetGameEpisode(Episode);
	GameState->SetGameMap(Map);
	GameState->SetGameSkill(Skill);

	// Load the level.
	DoLoadLevel();
}

void UDoomGameStateMachine::DoLoadGame()
{
	// Port of G_DoLoadGame() from g_game.c.
	// Stub - actual save/load requires the full WAD and archive systems.
	//
	// Original:
	//   Read save file, verify version
	//   Extract gameskill, gameepisode, gamemap, playeringame[]
	//   G_InitNew(gameskill, gameepisode, gamemap)
	//   Read leveltime
	//   P_UnArchivePlayers/World/Thinkers/Specials
	//   Verify save integrity marker

	if (!GameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Load game requested from '%s' (stub)"), *SaveFileName);
	GameState->SetGameAction(EDoomGameAction::Nothing);

	// TODO: Implement save game loading when archive system is ported.
}

void UDoomGameStateMachine::DoSaveGame()
{
	// Port of G_DoSaveGame() from g_game.c.
	// Stub - actual save/load requires the full WAD and archive systems.
	//
	// Original:
	//   Write save description, version
	//   Write gameskill, gameepisode, gamemap, playeringame[], leveltime
	//   P_ArchivePlayers/World/Thinkers/Specials
	//   Write consistency marker
	//   M_WriteFile()

	if (!GameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Save game to slot %d '%s' (stub)"), SaveGameSlot, *SaveDescription);
	GameState->SetGameAction(EDoomGameAction::Nothing);

	// TODO: Implement save game writing when archive system is ported.
}

void UDoomGameStateMachine::DoCompleted()
{
	// Port of G_DoCompleted() from g_game.c.
	//
	// Original:
	//   gameaction = ga_nothing;
	//   For each player: G_PlayerFinishLevel(i)  (clear powers, cards)
	//   Stop automap
	//   Check for episode end (map 8 in DOOM 1 -> victory)
	//   Calculate next map (handling secret exits)
	//   Fill wminfo struct for intermission
	//   gamestate = GS_INTERMISSION
	//   WI_Start(&wminfo)

	if (!GameState)
	{
		return;
	}

	GameState->SetGameAction(EDoomGameAction::Nothing);
	const EDoomGameState OldState = GameState->GetGameState();

	const int32 CurrentMap = GameState->GetGameMap();
	const int32 CurrentEpisode = GameState->GetGameEpisode();

	// Check for victory condition (end of episode in DOOM 1).
	if (GameState->GetGameMode() != EDoomGameMode::Commercial)
	{
		if (CurrentMap == 8)
		{
			// Episode end - trigger victory/finale.
			GameState->SetGameAction(EDoomGameAction::Victory);
			return;
		}
	}

	// Calculate next map.
	NextMap = CalculateNextMap(bSecretExit);

	// Transition to intermission.
	GameState->SetGameState(EDoomGameState::Intermission);
	GameState->SetViewActive(false);

	OnLevelCompleted.Broadcast(bSecretExit);
	OnGameStateChanged.Broadcast(OldState, EDoomGameState::Intermission);

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Level completed. E%dM%d -> Next map: %d (secret: %s)"),
		CurrentEpisode, CurrentMap, NextMap + 1,
		bSecretExit ? TEXT("yes") : TEXT("no"));

	// TODO: Start intermission screen (WI_Start equivalent).
}

void UDoomGameStateMachine::DoWorldDone()
{
	// Port of G_DoWorldDone() from g_game.c.
	//
	// Original:
	//   gamestate = GS_LEVEL;
	//   gamemap = wminfo.next + 1;
	//   G_DoLoadLevel();
	//   gameaction = ga_nothing;
	//   viewactive = true;

	if (!GameState)
	{
		return;
	}

	GameState->SetGameMap(NextMap + 1);
	DoLoadLevel();
	GameState->SetGameAction(EDoomGameAction::Nothing);
	GameState->SetViewActive(true);
}

void UDoomGameStateMachine::DoPlayDemo()
{
	// Port of G_DoPlayDemo() from g_game.c.
	// Stub - demo playback requires WAD lump reading.
	//
	// Original:
	//   Read demo header (version, skill, episode, map, deathmatch, etc.)
	//   G_InitNew(skill, episode, map)
	//   usergame = false; demoplayback = true;

	if (!GameState)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Play demo '%s' (stub)"), *DeferredDemoName);
	GameState->SetGameAction(EDoomGameAction::Nothing);

	// TODO: Implement demo playback when WAD system is connected.
}

void UDoomGameStateMachine::DoVictory()
{
	// Port of the ga_victory case in G_Ticker():
	//   F_StartFinale();
	//
	// Transitions to the finale game state.

	if (!GameState)
	{
		return;
	}

	const EDoomGameState OldState = GameState->GetGameState();
	GameState->SetGameState(EDoomGameState::Finale);
	GameState->SetGameAction(EDoomGameAction::Nothing);

	OnGameStateChanged.Broadcast(OldState, EDoomGameState::Finale);

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Victory! Starting finale."));

	// TODO: Start finale sequence (F_StartFinale equivalent).
}

int32 UDoomGameStateMachine::CalculateNextMap(bool bIsSecretExit) const
{
	// Port of next map calculation from G_DoCompleted() in g_game.c.
	// Returns 0-based map index (like wminfo.next).

	if (!GameState)
	{
		return 0;
	}

	const int32 CurrentMap = GameState->GetGameMap();
	const int32 CurrentEpisode = GameState->GetGameEpisode();
	const EDoomGameMode Mode = GameState->GetGameMode();

	if (Mode == EDoomGameMode::Commercial)
	{
		// DOOM 2 map progression.
		if (bIsSecretExit)
		{
			switch (CurrentMap)
			{
			case 15: return 30;  // MAP15 secret -> MAP31
			case 31: return 31;  // MAP31 secret -> MAP32
			default: break;
			}
		}

		switch (CurrentMap)
		{
		case 31:
		case 32:
			return 15;  // Secret levels return to MAP16
		default:
			return CurrentMap;  // Next sequential map (0-based, so gamemap is already next)
		}
	}
	else
	{
		// DOOM 1 map progression.
		if (bIsSecretExit)
		{
			return 8;  // Go to secret level (map 9, 0-based = 8)
		}

		if (CurrentMap == 9)
		{
			// Returning from secret level.
			switch (CurrentEpisode)
			{
			case 1: return 3;  // E1M9 -> E1M4
			case 2: return 5;  // E2M9 -> E2M6
			case 3: return 6;  // E3M9 -> E3M7
			case 4: return 2;  // E4M9 -> E4M3
			default: return 0;
			}
		}

		return CurrentMap;  // Next sequential map (0-based)
	}
}

// -------------------------------------------
// Demo recording/playback stubs
// -------------------------------------------

void UDoomGameStateMachine::BeginDemoRecording(const FString& DemoName)
{
	// Port of G_RecordDemo() + G_BeginRecording() from g_game.c.
	//
	// Original:
	//   G_RecordDemo: allocate buffer, set demorecording = true
	//   G_BeginRecording: write header (VERSION, skill, episode, map, etc.)

	RecordingDemoName = DemoName;

	if (GameState)
	{
		GameState->SetDemoRecording(true);
		GameState->SetUserGame(false);
	}

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Begin demo recording '%s' (stub)"), *DemoName);

	// TODO: Implement demo recording when buffer system is ported.
}

void UDoomGameStateMachine::StopDemoRecording()
{
	// Port of the recording path in G_CheckDemoStatus().
	//
	// Original:
	//   *demo_p++ = DEMOMARKER;
	//   M_WriteFile(demoname, demobuffer, demo_p - demobuffer);
	//   Z_Free(demobuffer);
	//   demorecording = false;

	if (GameState)
	{
		GameState->SetDemoRecording(false);
	}

	UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Stop demo recording '%s' (stub)"), *RecordingDemoName);
	RecordingDemoName.Empty();

	// TODO: Write demo file when buffer system is ported.
}

bool UDoomGameStateMachine::CheckDemoStatus()
{
	// Port of G_CheckDemoStatus() from g_game.c.
	//
	// Original:
	//   If timingdemo: report timing and exit
	//   If demoplayback: cleanup, advance demo sequence, return true
	//   If demorecording: write marker, save file, return false

	if (!GameState)
	{
		return false;
	}

	if (GameState->IsDemoPlayback())
	{
		if (GameState->IsSingleDemo())
		{
			// Would quit in the original.
			UE_LOG(LogTemp, Log, TEXT("DoomGameStateMachine: Single demo playback ended"));
			return false;
		}

		// Reset demo/net state.
		GameState->SetDemoPlayback(false);
		GameState->SetNetGame(false);
		GameState->SetDeathmatch(false);
		for (int32 i = 1; i < DOOM_MAXPLAYERS; ++i)
		{
			GameState->SetPlayerInGame(i, false);
		}
		GameState->SetRespawnParm(false);
		GameState->SetFastParm(false);
		GameState->SetNoMonsters(false);
		GameState->SetConsolePlayer(0);

		// Advance to next demo in sequence.
		// TODO: D_AdvanceDemo() equivalent.
		return true;
	}

	if (GameState->IsDemoRecording())
	{
		StopDemoRecording();
	}

	return false;
}
