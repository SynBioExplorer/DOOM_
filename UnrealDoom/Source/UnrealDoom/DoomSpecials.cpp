// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// Port of p_spec.c - sector and linedef special actions.

#include "DoomSpecials.h"
#include "DoomGameMode.h"
#include "GameLogic/DoomGameState.h"
#include "Core/DoomTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomSpecials, Log, All);

// =============================================================================
// Constructor
// =============================================================================

UDoomSpecials::UDoomSpecials()
{
}

// =============================================================================
// Initialization - P_SpawnSpecials equivalent
// =============================================================================

void UDoomSpecials::SpawnSpecials()
{
	UE_LOG(LogDoomSpecials, Log, TEXT("P_SpawnSpecials: Initializing level specials"));

	// Clear all active thinker lists from any previous level
	ActiveDoors.Empty();
	ActiveFloors.Empty();
	ActiveCeilings.Empty();
	ActivePlatforms.Empty();
	ActiveFlashes.Empty();
	ActiveStrobes.Empty();
	ActiveGlows.Empty();
	ActiveFlickers.Empty();
	ActiveButtons.Empty();
	ScrollingLines.Empty();
	LevelTimer = 0;

	// Scan all sectors for special types and spawn appropriate thinkers.
	// In the full implementation, this iterates over all sectors in the loaded
	// map data and creates light/effect thinkers based on sector->special:
	//
	// Sector special types (from original DOOM):
	//  1 = Light flicker (random)
	//  2 = Strobe fast
	//  3 = Strobe slow
	//  4 = Strobe fast + 20% damage (nukage)
	//  5 = 10% damage (hellslime)
	//  7 = 5% damage (slime w/o nukage)
	//  8 = Glow light
	//  9 = Secret (count toward secret total)
	// 10 = 30 seconds then close door
	// 11 = 20% damage + end level on death
	// 12 = Strobe slow synchronized
	// 13 = Strobe fast synchronized
	// 14 = 300 seconds then open door
	// 16 = 20% damage (super hellslime)
	// 17 = Fire flicker

	// TODO: Iterate loaded sectors from the level loader and create thinkers.
	// This requires access to the map data which will be provided by UDoomLevelLoader.
	// For now, the framework is ready to accept thinker creation.

	UE_LOG(LogDoomSpecials, Log, TEXT("P_SpawnSpecials: Complete"));
}

// =============================================================================
// Per-tic update - P_UpdateSpecials equivalent
// =============================================================================

void UDoomSpecials::UpdateSpecials()
{
	// Increment the level timer (used for animated texture cycling)
	LevelTimer++;

	// Update all active thinker types
	UpdateDoors();
	UpdateFloors();
	UpdateCeilings();
	UpdatePlatforms();
	UpdateLights();
	UpdateButtons();
	UpdateAnimations();
}

// =============================================================================
// Line activation
// =============================================================================

void UDoomSpecials::CrossSpecialLine(int32 LineIndex, int32 Side, int32 ActivatorIndex)
{
	// In the full implementation, this reads the line's Special field
	// and dispatches to the appropriate action. Walk-over triggers include:
	//
	// Doors: types 2, 3, 4, 16, 108, 109, etc.
	// Floors: types 5, 19, 30, 36, 37, 38, etc.
	// Platforms: types 10, 22, 53, 54, 87, 88, etc.
	// Teleporters: types 39, 97, 125, 126
	// Lights: types 35, 79, 80, 81, etc.
	// Exits: types 11, 51, 52, 124

	UE_LOG(LogDoomSpecials, Verbose, TEXT("CrossSpecialLine: line=%d side=%d activator=%d"),
		LineIndex, Side, ActivatorIndex);

	// TODO: Read line special from map data and dispatch
}

void UDoomSpecials::UseSpecialLine(int32 LineIndex, int32 Side, int32 ActivatorIndex)
{
	// Use-activated specials include:
	// Doors: types 1 (manual door), 26-28 (locked doors), 31-34, 99-105, 117
	// Switches: types 7, 9, 11, 14, 15, 18, 20-23, 29, 40-42, etc.
	// Exits: types 11, 51

	UE_LOG(LogDoomSpecials, Verbose, TEXT("UseSpecialLine: line=%d side=%d activator=%d"),
		LineIndex, Side, ActivatorIndex);

	// TODO: Read line special from map data and dispatch
}

void UDoomSpecials::ShootSpecialLine(int32 LineIndex)
{
	// Shoot-activated specials (gun triggers):
	// type 24 = Raise floor to lowest ceiling
	// type 46 = Open door (stay open)
	// type 47 = Raise floor to nearest (change texture)

	UE_LOG(LogDoomSpecials, Verbose, TEXT("ShootSpecialLine: line=%d"), LineIndex);

	// TODO: Read line special from map data and dispatch
}

// =============================================================================
// Door operations - EV_DoDoors equivalent
// =============================================================================

void UDoomSpecials::SpawnDoor(int32 LineIndex, EDoomDoorType Type)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("SpawnDoor: line=%d type=%d"), LineIndex, static_cast<int32>(Type));

	FDoomDoorThinker Door;
	Door.Type = Type;
	// Door.SectorIndex = ... (from line's backsector)

	// Set speed based on door type
	switch (Type)
	{
	case EDoomDoorType::BlazeRaise:
	case EDoomDoorType::BlazeOpen:
	case EDoomDoorType::BlazeClose:
		Door.Speed = DoomSpecialSpeeds::TURBOSPEED;
		break;
	default:
		Door.Speed = DoomSpecialSpeeds::VDOORSPEED;
		break;
	}

	// Set initial direction based on door type
	switch (Type)
	{
	case EDoomDoorType::Normal:
	case EDoomDoorType::BlazeRaise:
	case EDoomDoorType::Open:
	case EDoomDoorType::BlazeOpen:
		Door.Direction = 1; // Opening
		break;

	case EDoomDoorType::Close:
	case EDoomDoorType::BlazeClose:
		Door.Direction = -1; // Closing
		break;

	case EDoomDoorType::Close30ThenOpen:
		Door.Direction = -1; // Start by closing
		Door.TopCountDown = 30 * TICRATE; // Wait 30 seconds then open
		break;

	case EDoomDoorType::RaiseIn5Mins:
		Door.Direction = 2; // Wait before opening
		Door.TopCountDown = 5 * 60 * TICRATE; // 5 minutes
		break;
	}

	ActiveDoors.Add(MoveTemp(Door));
}

bool UDoomSpecials::SpawnLockedDoor(int32 LineIndex, EDoomDoorType Type, EDoomCard RequiredKey)
{
	// Check if the activating player has the required key
	// Player index 0 = console player (single player)
	if (!PlayerHasKey(0, RequiredKey))
	{
		// TODO: Play "oof" sound and display "You need a X key" message
		UE_LOG(LogDoomSpecials, Log, TEXT("Locked door: player lacks key %d"), static_cast<int32>(RequiredKey));
		return false;
	}

	SpawnDoor(LineIndex, Type);
	return true;
}

// =============================================================================
// Floor operations - EV_DoFloor equivalent
// =============================================================================

bool UDoomSpecials::DoFloor(int32 Tag, EDoomFloorType Type)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("DoFloor: tag=%d type=%d"), Tag, static_cast<int32>(Type));

	bool bActivated = false;

	// TODO: Iterate all sectors, find those with matching tag,
	// check they don't already have an active floor thinker,
	// then create a FDoomFloorThinker with appropriate parameters.
	//
	// For each matching sector:
	// FDoomFloorThinker Floor;
	// Floor.Type = Type;
	// Floor.SectorIndex = sectorIndex;
	// Floor.Speed = DoomSpecialSpeeds::FLOORSPEED;
	// Floor.FloorDestHeight = ... (calculated from surrounding sectors)
	// ActiveFloors.Add(MoveTemp(Floor));
	// bActivated = true;

	return bActivated;
}

bool UDoomSpecials::BuildStairs(int32 Tag, EDoomStairType Type)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("BuildStairs: tag=%d type=%d"), Tag, static_cast<int32>(Type));

	// Stair building creates a chain of floor movers that raise sectors
	// in sequence to create a staircase effect.
	// Step height: 8 units for Build8, 16 for Turbo16
	// Speed: FLOORSPEED for Build8, FLOORSPEED*4 for Turbo16

	// TODO: Implement stair building algorithm from P_BuildStairs

	return false;
}

// =============================================================================
// Ceiling operations - EV_DoCeiling equivalent
// =============================================================================

bool UDoomSpecials::DoCeiling(int32 Tag, EDoomCeilingType Type)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("DoCeiling: tag=%d type=%d"), Tag, static_cast<int32>(Type));

	// TODO: Iterate matching sectors and create ceiling thinkers

	return false;
}

void UDoomSpecials::CeilingCrushStop(int32 Tag)
{
	// Put all active ceiling crushers with matching tag into stasis
	for (FDoomCeilingThinker& Ceiling : ActiveCeilings)
	{
		if (Ceiling.Tag == Tag)
		{
			Ceiling.OldDirection = Ceiling.Direction;
			Ceiling.Direction = 0; // Stasis
			UE_LOG(LogDoomSpecials, Verbose, TEXT("Ceiling crusher stopped: tag=%d"), Tag);
		}
	}
}

// =============================================================================
// Platform / Lift operations - EV_DoPlat equivalent
// =============================================================================

bool UDoomSpecials::DoPlatform(int32 Tag, EDoomPlatformType Type, int32 Amount)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("DoPlatform: tag=%d type=%d amount=%d"),
		Tag, static_cast<int32>(Type), Amount);

	// TODO: Iterate matching sectors and create platform thinkers

	return false;
}

void UDoomSpecials::StopPlatform(int32 Tag)
{
	for (FDoomPlatformThinker& Plat : ActivePlatforms)
	{
		if (Plat.Tag == Tag)
		{
			Plat.OldStatus = Plat.Status;
			Plat.Status = 3; // In stasis
			UE_LOG(LogDoomSpecials, Verbose, TEXT("Platform stopped: tag=%d"), Tag);
		}
	}
}

// =============================================================================
// Light operations
// =============================================================================

void UDoomSpecials::TurnTagLightsOff(int32 Tag)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("TurnTagLightsOff: tag=%d"), Tag);

	// For each sector with matching tag, find the minimum light level
	// among neighboring sectors and set it to that.
	// TODO: Implement with access to map data
}

void UDoomSpecials::LightTurnOn(int32 Tag, int32 Bright)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("LightTurnOn: tag=%d bright=%d"), Tag, Bright);

	// For each sector with matching tag:
	// If Bright == 0, find max light among neighbors
	// Otherwise, set light to Bright
	// TODO: Implement with access to map data
}

// =============================================================================
// Teleporter - EV_Teleport equivalent
// =============================================================================

bool UDoomSpecials::Teleport(int32 Tag, int32 Side, int32 ThingIndex)
{
	// Teleport only works from the front side of a line
	if (Side == 1)
	{
		return false;
	}

	UE_LOG(LogDoomSpecials, Verbose, TEXT("Teleport: tag=%d thing=%d"), Tag, ThingIndex);

	// Find a sector with the matching tag, then find a teleport destination
	// thing (MT_TELEPORTMAN, DoomEdNum 14) in that sector.
	// Move the thing to the destination, set facing angle,
	// spawn teleport fog at both endpoints.

	// TODO: Implement with access to map data and thing list

	return false;
}

// =============================================================================
// Switch / Button
// =============================================================================

void UDoomSpecials::ChangeSwitchTexture(int32 LineIndex, bool bUseAgain)
{
	UE_LOG(LogDoomSpecials, Verbose, TEXT("ChangeSwitchTexture: line=%d useAgain=%d"),
		LineIndex, bUseAgain ? 1 : 0);

	// Swap the switch texture from SW1xxx to SW2xxx or vice versa.
	// If bUseAgain is true, start a timer to revert (making it a button).

	if (bUseAgain)
	{
		ActiveButtons.Add(LineIndex, BUTTONTIME);
	}

	// TODO: Actually swap the texture in the sidedef data
	// TODO: Play the switch activation sound
}

// =============================================================================
// Key card checking
// =============================================================================

bool UDoomSpecials::PlayerHasKey(int32 PlayerIndex, EDoomCard RequiredKey) const
{
	// In the full implementation, this checks the player's cards[] array.
	// DOOM also allows skull keys to substitute for card keys of the same color:
	// - BlueCard or BlueSkull
	// - YellowCard or YellowSkull
	// - RedCard or RedSkull

	// TODO: Access player state from the game state to check actual key inventory
	// For now, return false (no keys)

	UE_LOG(LogDoomSpecials, Verbose, TEXT("PlayerHasKey: player=%d key=%d (stub)"),
		PlayerIndex, static_cast<int32>(RequiredKey));

	return false;
}

// =============================================================================
// Per-tic update helpers
// =============================================================================

void UDoomSpecials::UpdateDoors()
{
	// Process each active door thinker (T_VerticalDoor equivalent)
	for (int32 i = ActiveDoors.Num() - 1; i >= 0; --i)
	{
		FDoomDoorThinker& Door = ActiveDoors[i];

		switch (Door.Direction)
		{
		case 0: // Waiting
			Door.TopCountDown--;
			if (Door.TopCountDown <= 0)
			{
				switch (Door.Type)
				{
				case EDoomDoorType::Normal:
				case EDoomDoorType::BlazeRaise:
					Door.Direction = -1; // Start closing
					break;

				case EDoomDoorType::Close30ThenOpen:
					Door.Direction = 1; // Start opening
					break;

				default:
					break;
				}
			}
			break;

		case 2: // Initial delay (RaiseIn5Mins)
			Door.TopCountDown--;
			if (Door.TopCountDown <= 0)
			{
				Door.Direction = 1; // Start opening
			}
			break;

		case -1: // Closing
			// TODO: Move sector ceiling down by Door.Speed
			// If ceiling reaches floor height:
			//   - For Close30ThenOpen: switch to opening after delay
			//   - For others: remove thinker
			// If something blocks: reverse for Normal/BlazeRaise types
			break;

		case 1: // Opening
			// TODO: Move sector ceiling up by Door.Speed
			// If ceiling reaches Door.TopHeight:
			//   - For Open/BlazeOpen: remove thinker (stays open)
			//   - For Normal/BlazeRaise: switch to waiting
			//     Door.TopCountDown = DoomSpecialSpeeds::VDOORWAIT;
			//     Door.Direction = 0;
			break;
		}
	}
}

void UDoomSpecials::UpdateFloors()
{
	// Process each active floor thinker (T_MoveFloor equivalent)
	for (int32 i = ActiveFloors.Num() - 1; i >= 0; --i)
	{
		FDoomFloorThinker& Floor = ActiveFloors[i];

		// TODO: Move sector floor by Floor.Speed in Floor.Direction
		// Check for crush if bCrush is true
		// Remove thinker when destination is reached
		// Apply texture/type change if specified
	}
}

void UDoomSpecials::UpdateCeilings()
{
	// Process each active ceiling thinker (T_MoveCeiling equivalent)
	for (int32 i = ActiveCeilings.Num() - 1; i >= 0; --i)
	{
		FDoomCeilingThinker& Ceiling = ActiveCeilings[i];

		if (Ceiling.Direction == 0) // In stasis
		{
			continue;
		}

		// TODO: Move sector ceiling by Ceiling.Speed in Ceiling.Direction
		// For crushers: reverse direction when reaching top/bottom
		// Apply damage to things caught in crusher
	}
}

void UDoomSpecials::UpdatePlatforms()
{
	// Process each active platform thinker (T_PlatRaise equivalent)
	for (int32 i = ActivePlatforms.Num() - 1; i >= 0; --i)
	{
		FDoomPlatformThinker& Plat = ActivePlatforms[i];

		switch (Plat.Status)
		{
		case 0: // Going up
			// TODO: Move sector floor up by Plat.Speed
			// If reached Plat.High: switch to waiting
			break;

		case 1: // Going down
			// TODO: Move sector floor down by Plat.Speed
			// If reached Plat.Low: switch to waiting (or remove for perpetual)
			break;

		case 2: // Waiting
			Plat.Count--;
			if (Plat.Count <= 0)
			{
				// Switch direction
				if (Plat.Status == 0)
				{
					Plat.Status = 1; // Now go down
				}
				else
				{
					Plat.Status = 0; // Now go up
				}
				Plat.Count = Plat.Wait;
			}
			break;

		case 3: // In stasis
			break;
		}
	}
}

void UDoomSpecials::UpdateLights()
{
	// Update light flashes (T_LightFlash equivalent)
	for (FDoomLightFlash& Flash : ActiveFlashes)
	{
		Flash.Count--;
		if (Flash.Count <= 0)
		{
			// TODO: Toggle sector light level between MaxLight and MinLight
			// Set Count to random time (MaxTime or MinTime)
			Flash.Count = (FMath::RandBool()) ? Flash.MaxTime : Flash.MinTime;
		}
	}

	// Update strobe lights (T_StrobeFlash equivalent)
	for (FDoomStrobeFlash& Strobe : ActiveStrobes)
	{
		Strobe.Count--;
		if (Strobe.Count <= 0)
		{
			// TODO: Toggle sector light between bright and dark
			// If currently bright, set to MinLight and Count = DarkTime
			// If currently dark, set to MaxLight and Count = BrightTime
		}
	}

	// Update glowing lights (T_Glow equivalent)
	for (FDoomGlowingLight& Glow : ActiveGlows)
	{
		// TODO: Adjust sector light level by GLOWSPEED in Glow.Direction
		// Reverse direction when hitting MinLight or MaxLight
	}

	// Update fire flickers (T_FireFlicker equivalent)
	for (FDoomFireFlicker& Flicker : ActiveFlickers)
	{
		Flicker.Count--;
		if (Flicker.Count <= 0)
		{
			// TODO: Set sector light to MaxLight - random(0..15)
			Flicker.Count = 4; // 4 tics between flickers
		}
	}
}

void UDoomSpecials::UpdateButtons()
{
	// Decrement button timers and revert switch textures when expired
	TArray<int32> ExpiredButtons;

	for (auto& Pair : ActiveButtons)
	{
		Pair.Value--;
		if (Pair.Value <= 0)
		{
			ExpiredButtons.Add(Pair.Key);
			// TODO: Revert the switch texture from SW2xxx back to SW1xxx
			// TODO: Play switch deactivation sound
		}
	}

	for (const int32 LineIndex : ExpiredButtons)
	{
		ActiveButtons.Remove(LineIndex);
	}
}

void UDoomSpecials::UpdateAnimations()
{
	// Animated flats cycle every 8 tics (original DOOM behavior)
	// The flat animation table defines sequences like:
	// NUKAGE1 -> NUKAGE2 -> NUKAGE3 -> NUKAGE1 (cycling)
	// FWATER1 -> FWATER2 -> FWATER3 -> FWATER4 (cycling)
	//
	// Scrolling walls (line type 48) scroll their texture offset
	// by 1 unit per tic.

	// TODO: Implement animated flat cycling and wall scrolling
	// when texture system is available

	// Update scrolling lines (type 48 = scroll left)
	// for (int32 LineIndex : ScrollingLines)
	// {
	//     // Add 1 to the sidedef texture offset each tic
	// }
}
