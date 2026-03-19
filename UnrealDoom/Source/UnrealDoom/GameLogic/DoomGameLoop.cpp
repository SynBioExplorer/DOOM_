#include "DoomGameLoop.h"
#include "DoomGameState.h"
#include "DoomThinker.h"

UDoomGameLoop::UDoomGameLoop()
	: TimeAccumulator(0.0f)
	, bAdvanceDemo(false)
{
	// Zero-initialize the net command buffers.
	FMemory::Memzero(NetCmds, sizeof(NetCmds));
}

void UDoomGameLoop::Initialize(UDoomGameState* InGameState, UDoomThinkerManager* InThinkerManager)
{
	GameState = InGameState;
	ThinkerManager = InThinkerManager;
	TimeAccumulator = 0.0f;
	bAdvanceDemo = false;
	FMemory::Memzero(NetCmds, sizeof(NetCmds));
}

void UDoomGameLoop::Tick(float DeltaTime)
{
	if (!IsInitialized())
	{
		return;
	}

	// Accumulate real time.
	// This replaces the original I_GetTime() based timing.
	// In D_DoomLoop: nowtime = I_GetTime()/ticdup; newtics = nowtime - gametime;
	TimeAccumulator += DeltaTime;

	if (GameState->IsSingleTics())
	{
		// Single tic mode (debug): run exactly one tic per frame.
		// Original: I_StartTic(); D_ProcessEvents();
		//           G_BuildTiccmd(&netcmds[consoleplayer][maketic%BACKUPTICS]);
		//           M_Ticker(); G_Ticker(); gametic++; maketic++;
		ProcessEvents();

		FDoomTicCmd Cmd;
		BuildTicCmd(Cmd);

		const int32 ConsolePlayer = GameState->GetConsolePlayer();
		const int32 MakeTic = GameState->GetMakeTic();
		SetNetCmd(ConsolePlayer, MakeTic % DOOM_BACKUPTICS, Cmd);
		GameState->IncrementMakeTic();

		if (bAdvanceDemo)
		{
			HandleAdvanceDemo();
		}

		RunTickers();
		GameState->IncrementGameTic();

		// Reset accumulator to prevent drift.
		TimeAccumulator = 0.0f;
	}
	else
	{
		// Normal mode: accumulator-based fixed timestep at 35 tics/sec.
		// This replaces TryRunTics() from d_net.c.
		int32 TicsToRun = 0;

		while (TimeAccumulator >= TicDuration && TicsToRun < MaxTicsPerFrame)
		{
			TimeAccumulator -= TicDuration;
			TicsToRun++;
		}

		if (TicsToRun > 0)
		{
			// Build tic commands for the accumulated tics.
			// Original NetUpdate: for (i=0; i<newtics; i++) { G_BuildTiccmd(&localcmds[maketic%BACKUPTICS]); maketic++; }
			ProcessEvents();

			for (int32 i = 0; i < TicsToRun; ++i)
			{
				FDoomTicCmd Cmd;
				BuildTicCmd(Cmd);

				const int32 ConsolePlayer = GameState->GetConsolePlayer();
				const int32 MakeTic = GameState->GetMakeTic();
				SetNetCmd(ConsolePlayer, MakeTic % DOOM_BACKUPTICS, Cmd);
				GameState->IncrementMakeTic();
			}

			TryRunTics(TicsToRun);
		}
	}
}

void UDoomGameLoop::ProcessEvents()
{
	// Port of D_ProcessEvents() from d_main.c.
	//
	// Original:
	//   for (; eventtail != eventhead; eventtail = (++eventtail)&(MAXEVENTS-1))
	//   {
	//       ev = &events[eventtail];
	//       if (M_Responder(ev)) continue;  // menu ate the event
	//       G_Responder(ev);
	//   }
	//
	// In UE5, input events are handled by the engine's input system.
	// This is a stub that will be connected to UE5's input pipeline
	// via the DoomInputProcessor (see Input/ module).
	//
	// TODO: Integrate with UE5 input system to convert Enhanced Input
	// actions into DOOM event_t equivalents and pass through the
	// responder chain (Menu -> Game -> HUD -> Status Bar -> Automap).
}

void UDoomGameLoop::BuildTicCmd(FDoomTicCmd& OutCmd)
{
	// Port of G_BuildTiccmd() from g_game.c.
	//
	// Original builds a ticcmd from keyboard, mouse, joystick state:
	//   - Check strafe/speed keys
	//   - Accelerative turning
	//   - Forward/backward movement
	//   - Strafing
	//   - Mouse look and movement
	//   - Button state (fire, use, weapon change)
	//   - Special buttons (pause, save)
	//
	// In UE5, we will receive processed input from the Enhanced Input system.
	// This stub initializes a clean command.

	OutCmd.Clear();

	if (!GameState)
	{
		return;
	}

	// Set consistency check value.
	// Original: cmd->consistancy = consistancy[consoleplayer][maketic%BACKUPTICS];
	// TODO: Implement consistency tracking for network play.
	OutCmd.Consistancy = 0;

	// TODO: Sample UE5 input state and fill in:
	// - OutCmd.ForwardMove (from forward/backward input axis)
	// - OutCmd.SideMove (from strafe input axis)
	// - OutCmd.AngleTurn (from turn input axis)
	// - OutCmd.Buttons (from action mappings: fire, use, weapon change)
	// - OutCmd.ChatChar (from chat input)
}

void UDoomGameLoop::RunTickers()
{
	// Port of the main G_Ticker() dispatch from g_game.c.
	//
	// The original G_Ticker does:
	// 1. Handle player reborns
	// 2. Process game actions (ga_loadlevel, ga_newgame, etc.)
	// 3. Copy net commands to player structs
	// 4. Check for special buttons (pause, save)
	// 5. Dispatch to state-specific ticker

	if (!GameState || !ThinkerManager)
	{
		return;
	}

	const int32 GameTic = GameState->GetGameTic();

	// Fire pre-tic delegate.
	OnPreGameTic.Broadcast(GameTic);

	// Step 1: Player reborns would be handled here.
	// Original: for (i=0; i<MAXPLAYERS; i++)
	//               if (playeringame[i] && players[i].playerstate == PST_REBORN)
	//                   G_DoReborn(i);
	// TODO: Implement when player system is ported.

	// Step 2: Process pending game actions.
	// Original: while (gameaction != ga_nothing) { switch (gameaction) { ... } }
	// This is handled by the DoomGameStateMachine (see DoomGameStateMachine.h).

	// Step 3: Copy net commands to player command structs.
	// Original: for (i=0; i<MAXPLAYERS; i++)
	//               if (playeringame[i])
	//                   memcpy(&players[i].cmd, &netcmds[i][buf], sizeof(ticcmd_t));
	// TODO: Implement when player system is ported.

	// Step 4: Check for special buttons (pause, save).
	// Original: checks BT_SPECIAL, BTS_PAUSE, BTS_SAVEGAME.
	// TODO: Implement when player system is ported.

	// Step 5: Dispatch based on game state.
	// Original:
	//   switch (gamestate) {
	//     case GS_LEVEL:       P_Ticker(); ST_Ticker(); AM_Ticker(); HU_Ticker(); break;
	//     case GS_INTERMISSION: WI_Ticker(); break;
	//     case GS_FINALE:      F_Ticker(); break;
	//     case GS_DEMOSCREEN:  D_PageTicker(); break;
	//   }
	switch (GameState->GetGameState())
	{
	case EDoomGameState::Level:
		// P_Ticker equivalent: run player think, thinkers, specials, respawn.
		if (!GameState->IsPaused())
		{
			// Pause check from P_Ticker:
			//   if (paused) return;
			//   if (!netgame && menuactive && !demoplayback && players[consoleplayer].viewz != 1) return;
			if (!GameState->IsNetGame() && GameState->IsMenuActive() && !GameState->IsDemoPlayback())
			{
				// Paused in menu during single player - skip ticking.
				break;
			}

			// Run all thinkers (P_RunThinkers equivalent).
			ThinkerManager->RunThinkers();

			// Increment level time (for par time calculation).
			GameState->IncrementLevelTime();
		}
		// TODO: ST_Ticker(), AM_Ticker(), HU_Ticker() equivalents.
		break;

	case EDoomGameState::Intermission:
		// TODO: WI_Ticker() equivalent.
		break;

	case EDoomGameState::Finale:
		// TODO: F_Ticker() equivalent.
		break;

	case EDoomGameState::DemoScreen:
		// TODO: D_PageTicker() equivalent.
		break;
	}

	// Fire post-tic delegate.
	OnPostGameTic.Broadcast(GameTic);
}

void UDoomGameLoop::TryRunTics(int32 TicsToRun)
{
	// Simplified port of TryRunTics() from d_net.c.
	//
	// The original TryRunTics:
	// 1. Gets real tics from I_GetTime()
	// 2. Calls NetUpdate() to build commands and exchange packets
	// 3. Finds lowest tic across all network nodes
	// 4. Decides how many tics to run
	// 5. Waits for new tics if needed
	// 6. Runs count * ticdup tics
	//
	// In this UE5 port, timing is handled by the accumulator in Tick(),
	// and network synchronization is deferred. We simply run the
	// requested number of tics.

	if (!GameState)
	{
		return;
	}

	const int32 TicDup = FMath::Max(1, GameState->GetTicDup());

	for (int32 Count = 0; Count < TicsToRun; ++Count)
	{
		// Run ticdup iterations per logical tic.
		// Original: for (i=0; i<ticdup; i++) { ... G_Ticker(); gametic++; }
		for (int32 Dup = 0; Dup < TicDup; ++Dup)
		{
			if (bAdvanceDemo)
			{
				HandleAdvanceDemo();
			}

			RunTickers();
			GameState->IncrementGameTic();

			// For duplicated tics, clear transient command data.
			// Original: if (i != ticdup-1) { cmd->chatchar = 0; if (cmd->buttons & BT_SPECIAL) cmd->buttons = 0; }
			if (Dup != TicDup - 1)
			{
				const int32 Buf = (GameState->GetGameTic() / TicDup) % DOOM_BACKUPTICS;
				for (int32 Player = 0; Player < DOOM_MAXPLAYERS; ++Player)
				{
					FDoomTicCmd& Cmd = NetCmds[Player][Buf];
					Cmd.ChatChar = 0;
					if (Cmd.Buttons & DoomButtons::BT_SPECIAL)
					{
						Cmd.Buttons = 0;
					}
				}
			}
		}
	}
}

void UDoomGameLoop::ExecuteSingleTic()
{
	if (bAdvanceDemo)
	{
		HandleAdvanceDemo();
	}

	RunTickers();

	if (GameState)
	{
		GameState->IncrementGameTic();
	}
}

void UDoomGameLoop::HandleAdvanceDemo()
{
	// Port of D_DoAdvanceDemo() from d_main.c.
	// Cycles through the demo sequence (title screen, demos, credits).
	//
	// Original sets playerstate, advancedemo=false, usergame=false, paused=false,
	// gameaction=ga_nothing, then cycles through demosequence to set pagetic,
	// gamestate, pagename, or start a demo.
	//
	// TODO: Implement demo sequence cycling when demo system is fully ported.
	bAdvanceDemo = false;

	if (GameState)
	{
		GameState->SetUserGame(false);
		GameState->SetPaused(false);
		GameState->SetGameAction(EDoomGameAction::Nothing);
	}
}

const FDoomTicCmd& UDoomGameLoop::GetNetCmd(int32 PlayerIndex, int32 TicIndex) const
{
	static const FDoomTicCmd EmptyCmd;
	if (PlayerIndex < 0 || PlayerIndex >= DOOM_MAXPLAYERS)
	{
		return EmptyCmd;
	}
	const int32 Slot = ((TicIndex % DOOM_BACKUPTICS) + DOOM_BACKUPTICS) % DOOM_BACKUPTICS;
	return NetCmds[PlayerIndex][Slot];
}

void UDoomGameLoop::SetNetCmd(int32 PlayerIndex, int32 TicIndex, const FDoomTicCmd& Cmd)
{
	if (PlayerIndex < 0 || PlayerIndex >= DOOM_MAXPLAYERS)
	{
		return;
	}
	const int32 Slot = ((TicIndex % DOOM_BACKUPTICS) + DOOM_BACKUPTICS) % DOOM_BACKUPTICS;
	NetCmds[PlayerIndex][Slot] = Cmd;
}
