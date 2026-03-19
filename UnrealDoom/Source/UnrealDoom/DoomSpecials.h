#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomSpecials - Port of p_spec.c / p_spec.h
// Handles all sector and linedef special actions:
// doors, floors, ceilings, lifts, lights, teleporters, switches.

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/DoomTypes.h"
#include "DoomSpecials.generated.h"

class UDoomGameState;
class ADoomGameMode;

// =============================================================================
// Door types - from vldoor_e in p_spec.h
// =============================================================================

UENUM(BlueprintType)
enum class EDoomDoorType : uint8
{
	Normal = 0,         // vld_normal - Open, wait, close
	Close30ThenOpen,    // vld_close30ThenOpen
	Close,              // vld_close
	Open,               // vld_open - Open and stay open
	RaiseIn5Mins,       // vld_raiseIn5Mins
	BlazeRaise,         // vld_blazeRaise - Fast door, open, wait, close
	BlazeOpen,          // vld_blazeOpen - Fast open, stay open
	BlazeClose          // vld_blazeClose - Fast close
};

// =============================================================================
// Floor action types - from floor_e in p_spec.h
// =============================================================================

UENUM(BlueprintType)
enum class EDoomFloorType : uint8
{
	LowerFloor = 0,             // lowerFloor
	LowerFloorToLowest,         // lowerFloorToLowest
	TurboLower,                 // turboLower
	RaiseFloor,                 // raiseFloor
	RaiseFloorToNearest,        // raiseFloorToNearest
	RaiseToTexture,             // raiseToTexture
	LowerAndChange,             // lowerAndChange
	RaiseFloor24,               // raiseFloor24
	RaiseFloor24AndChange,      // raiseFloor24AndChange
	RaiseFloorCrush,            // raiseFloorCrush
	RaiseFloorTurbo,            // raiseFloorTurbo
	DonutRaise,                 // donutRaise
	RaiseFloor512               // raiseFloor512
};

// =============================================================================
// Ceiling action types - from ceiling_e in p_spec.h
// =============================================================================

UENUM(BlueprintType)
enum class EDoomCeilingType : uint8
{
	LowerToFloor = 0,           // lowerToFloor
	RaiseToHighest,             // raiseToHighest
	LowerAndCrush,              // lowerAndCrush
	CrushAndRaise,              // crushAndRaise
	FastCrushAndRaise,          // fastCrushAndRaise
	SilentCrushAndRaise         // silentCrushAndRaise
};

// =============================================================================
// Platform (lift) types - from plattype_e in p_spec.h
// =============================================================================

UENUM(BlueprintType)
enum class EDoomPlatformType : uint8
{
	RaiseToNearestAndChange = 0,   // raiseToNearestAndChange
	RaiseAndChange,                // raiseAndChange
	DownWaitUpStay,                // downWaitUpStay
	BlazeDWUS,                     // blazeDWUS (fast lift)
	PerpetualRaise                 // perpetualRaise
};

// =============================================================================
// Staircase types - from stair_e in p_spec.h
// =============================================================================

UENUM(BlueprintType)
enum class EDoomStairType : uint8
{
	Build8 = 0,        // build8 - 8 unit step height
	Turbo16             // turbo16 - 16 unit step height, fast
};

// =============================================================================
// Thinker structures for active specials
// =============================================================================

/**
 * FDoomDoorThinker - Active door state (vldoor_t from p_spec.h)
 */
USTRUCT(BlueprintType)
struct FDoomDoorThinker
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	EDoomDoorType Type = EDoomDoorType::Normal;

	/** Index of the sector this door controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	/** Top height of the door (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 TopHeight = 0;

	/** Movement speed (fixed-point units per tic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Speed = 0;

	/**
	 * Current direction: 1 = opening, 0 = waiting, -1 = closing, 2 = initial delay
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Direction = 0;

	/** Wait counter (tics remaining before next state change) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 TopCountDown = 0;
};

/**
 * FDoomFloorThinker - Active floor mover state (floormove_t)
 */
USTRUCT(BlueprintType)
struct FDoomFloorThinker
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	EDoomFloorType Type = EDoomFloorType::LowerFloor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	/** Are we crushing things? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	bool bCrush = false;

	/** Direction: 1 = up, -1 = down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Direction = 0;

	/** New floor texture index after move completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 NewSpecial = 0;

	/** New floor texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Texture = 0;

	/** Destination height (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 FloorDestHeight = 0;

	/** Movement speed (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Speed = 0;
};

/**
 * FDoomCeilingThinker - Active ceiling mover / crusher state (ceiling_t)
 */
USTRUCT(BlueprintType)
struct FDoomCeilingThinker
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	EDoomCeilingType Type = EDoomCeilingType::LowerToFloor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	/** Bottom height (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 BottomHeight = 0;

	/** Top height (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 TopHeight = 0;

	/** Movement speed (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Speed = 0;

	/** Old movement speed (for crush reversal) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 OldSpeed = 0;

	/** Are we crushing things? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	bool bCrush = false;

	/** Direction: 1 = up, -1 = down, 0 = in stasis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Direction = 0;

	/** Sector tag for this ceiling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Tag = 0;

	/** Old direction (for stasis/resume) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 OldDirection = 0;
};

/**
 * FDoomPlatformThinker - Active platform/lift state (plat_t)
 */
USTRUCT(BlueprintType)
struct FDoomPlatformThinker
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	EDoomPlatformType Type = EDoomPlatformType::DownWaitUpStay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	/** Movement speed (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Speed = 0;

	/** Low height (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Low = 0;

	/** High height (fixed-point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 High = 0;

	/** Wait counter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Wait = 0;

	/** Count (tics remaining in wait state) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Count = 0;

	/** Status: 0 = up, 1 = down, 2 = waiting, 3 = in stasis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Status = 0;

	/** Old status (for stasis/resume) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 OldStatus = 0;

	/** Is this a crusher? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	bool bCrush = false;

	/** Tag for identification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Tag = 0;
};

/**
 * FDoomLightFlash - Broken light flashing state (lightflash_t)
 */
USTRUCT(BlueprintType)
struct FDoomLightFlash
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MaxLight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MinLight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MaxTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MinTime = 0;
};

/**
 * FDoomStrobeFlash - Strobe light state (strobe_t)
 */
USTRUCT(BlueprintType)
struct FDoomStrobeFlash
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MinLight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MaxLight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 DarkTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 BrightTime = 0;
};

/**
 * FDoomGlowingLight - Glowing light state (glow_t)
 */
USTRUCT(BlueprintType)
struct FDoomGlowingLight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MinLight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MaxLight = 0;

	/** Direction: 1 = brightening, -1 = dimming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Direction = 1;
};

/**
 * FDoomFireFlicker - Fire flicker light state (fireflicker_t)
 */
USTRUCT(BlueprintType)
struct FDoomFireFlicker
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 SectorIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MaxLight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Specials")
	int32 MinLight = 0;
};

// =============================================================================
// Speed constants from p_spec.h
// =============================================================================

namespace DoomSpecialSpeeds
{
	constexpr int32 VDOORSPEED = 2 * 65536;         // VDOORSPEED (2 * FRACUNIT)
	constexpr int32 VDOORWAIT = 150;                 // Tics to wait before closing
	constexpr int32 FLOORSPEED = 65536;              // FLOORSPEED (FRACUNIT)
	constexpr int32 CEILSPEED = 65536;               // CEILSPEED (FRACUNIT)
	constexpr int32 PLATSPEED = 65536;               // PLATSPEED (FRACUNIT)
	constexpr int32 PLATWAIT = 3;                    // Seconds to wait (converted to tics)
	constexpr int32 GLOWSPEED = 8;                   // Light units per tic
	constexpr int32 STROBEBRIGHT = 5;                // Tics for bright phase
	constexpr int32 FASTDARK = 15;                   // Tics for fast strobe dark
	constexpr int32 SLOWDARK = 35;                   // Tics for slow strobe dark
	constexpr int32 TURBOSPEED = 4 * 65536;          // Fast door/floor speed
}

/**
 * UDoomSpecials - Manages all sector and linedef special actions.
 *
 * Port of p_spec.c from the original DOOM source. Handles:
 * - Doors: normal, blazing, locked (key card checking)
 * - Floors: raise, lower, crush
 * - Ceilings: raise, lower, crush
 * - Platforms/Lifts: raise, lower, perpetual
 * - Lights: strobe, glow, flicker, fire
 * - Teleporters
 * - Switches and button activation
 *
 * Active specials are tracked as thinker arrays and updated each game tic.
 */
UCLASS(BlueprintType, ClassGroup = "Doom")
class UNREALDOOM_API UDoomSpecials : public UObject
{
	GENERATED_BODY()

public:
	UDoomSpecials();

	// =========================================================================
	// Initialization
	// =========================================================================

	/**
	 * Initialize all specials after a level is loaded.
	 * Scans sectors for special types and creates the appropriate thinkers.
	 * Equivalent to P_SpawnSpecials.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials")
	void SpawnSpecials();

	// =========================================================================
	// Per-tic update
	// =========================================================================

	/**
	 * Update all active specials. Called once per game tic (35 Hz).
	 * Equivalent to P_UpdateSpecials.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials")
	void UpdateSpecials();

	// =========================================================================
	// Line activation (called by P_UseSpecialLine / P_CrossSpecialLine)
	// =========================================================================

	/**
	 * Activate a linedef special when a player crosses it.
	 * @param LineIndex - Index of the linedef crossed
	 * @param Side - Which side was crossed (0 = front, 1 = back)
	 * @param ActivatorIndex - Index of the thing that crossed it
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials")
	void CrossSpecialLine(int32 LineIndex, int32 Side, int32 ActivatorIndex);

	/**
	 * Activate a linedef special when a player uses (presses use on) it.
	 * @param LineIndex - Index of the linedef used
	 * @param Side - Which side was used (0 = front, 1 = back)
	 * @param ActivatorIndex - Index of the thing that used it
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials")
	void UseSpecialLine(int32 LineIndex, int32 Side, int32 ActivatorIndex);

	/**
	 * Activate a linedef special when shot.
	 * @param LineIndex - Index of the linedef shot
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials")
	void ShootSpecialLine(int32 LineIndex);

	// =========================================================================
	// Door operations
	// =========================================================================

	/**
	 * Spawn a vertical door on a sector.
	 * @param LineIndex - The linedef that triggered the door
	 * @param Type - Door behavior type
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Doors")
	void SpawnDoor(int32 LineIndex, EDoomDoorType Type);

	/**
	 * Spawn a locked door that requires a key card.
	 * @param LineIndex - The linedef that triggered the door
	 * @param Type - Door behavior type
	 * @param RequiredKey - The key card needed to open
	 * @return true if the player has the key and the door was activated
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Doors")
	bool SpawnLockedDoor(int32 LineIndex, EDoomDoorType Type, EDoomCard RequiredKey);

	// =========================================================================
	// Floor operations
	// =========================================================================

	/**
	 * Activate a floor mover on sectors with the given tag.
	 * @param Tag - Sector tag to match
	 * @param Type - Floor action type
	 * @return true if any sectors were activated
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Floors")
	bool DoFloor(int32 Tag, EDoomFloorType Type);

	/**
	 * Build stairs starting from a sector.
	 * @param Tag - Starting sector tag
	 * @param Type - Stair type (normal or turbo)
	 * @return true if stairs were built
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Floors")
	bool BuildStairs(int32 Tag, EDoomStairType Type);

	// =========================================================================
	// Ceiling operations
	// =========================================================================

	/**
	 * Activate a ceiling mover on sectors with the given tag.
	 * @param Tag - Sector tag to match
	 * @param Type - Ceiling action type
	 * @return true if any sectors were activated
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Ceilings")
	bool DoCeiling(int32 Tag, EDoomCeilingType Type);

	/**
	 * Put active ceilings with the given tag into stasis.
	 * @param Tag - Sector tag to match
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Ceilings")
	void CeilingCrushStop(int32 Tag);

	// =========================================================================
	// Platform / Lift operations
	// =========================================================================

	/**
	 * Activate a platform/lift on sectors with the given tag.
	 * @param Tag - Sector tag to match
	 * @param Type - Platform type
	 * @param Amount - Height change amount (used by some types)
	 * @return true if any platforms were activated
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Platforms")
	bool DoPlatform(int32 Tag, EDoomPlatformType Type, int32 Amount);

	/**
	 * Stop active platforms with the given tag.
	 * @param Tag - Tag of platforms to stop
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Platforms")
	void StopPlatform(int32 Tag);

	// =========================================================================
	// Light operations
	// =========================================================================

	/**
	 * Turn lights off in sectors with the given tag.
	 * Sets light level to the minimum neighboring light level.
	 * @param Tag - Sector tag to match
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Lights")
	void TurnTagLightsOff(int32 Tag);

	/**
	 * Turn lights on in sectors with the given tag.
	 * Sets light level to the maximum neighboring light level.
	 * @param Tag - Sector tag to match
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Lights")
	void LightTurnOn(int32 Tag, int32 Bright);

	// =========================================================================
	// Teleporter
	// =========================================================================

	/**
	 * Teleport a thing to the destination matching the given tag.
	 * @param Tag - Sector tag of the teleport destination
	 * @param Side - Line side crossed (teleport only works from front)
	 * @param ThingIndex - Index of the thing to teleport
	 * @return true if teleport occurred
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Teleporter")
	bool Teleport(int32 Tag, int32 Side, int32 ThingIndex);

	// =========================================================================
	// Switch / Button
	// =========================================================================

	/**
	 * Change a switch texture (activated -> deactivated or vice versa).
	 * @param LineIndex - The switch linedef
	 * @param bUseAgain - If true, start a timer to revert the switch (button)
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials|Switches")
	void ChangeSwitchTexture(int32 LineIndex, bool bUseAgain);

	// =========================================================================
	// Key card checking
	// =========================================================================

	/**
	 * Check if a player has the required key card for a locked door.
	 * Checks both card and skull variants of each color.
	 *
	 * @param PlayerIndex - Player to check
	 * @param RequiredKey - The key card required
	 * @return true if the player has the required key
	 */
	UFUNCTION(BlueprintPure, Category = "Doom|Specials|Keys")
	bool PlayerHasKey(int32 PlayerIndex, EDoomCard RequiredKey) const;

	// =========================================================================
	// Utility
	// =========================================================================

	/** Set the owning game mode for access to game state and level data. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Specials")
	void SetGameMode(ADoomGameMode* InGameMode) { OwnerGameMode = InGameMode; }

protected:
	// =========================================================================
	// Active thinker lists
	// =========================================================================

	/** Active door thinkers */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomDoorThinker> ActiveDoors;

	/** Active floor movers */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomFloorThinker> ActiveFloors;

	/** Active ceiling movers / crushers */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomCeilingThinker> ActiveCeilings;

	/** Active platforms / lifts */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomPlatformThinker> ActivePlatforms;

	/** Active light flashes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomLightFlash> ActiveFlashes;

	/** Active strobe lights */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomStrobeFlash> ActiveStrobes;

	/** Active glowing lights */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomGlowingLight> ActiveGlows;

	/** Active fire flickers */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<FDoomFireFlicker> ActiveFlickers;

	// =========================================================================
	// Button state (for switches that revert after a delay)
	// =========================================================================

	/** Maximum number of active buttons */
	static constexpr int32 MAXBUTTONS = 16;

	/** Button timers: line index -> tics remaining before texture revert */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TMap<int32, int32> ActiveButtons;

	/** Button texture timer duration in tics (1 second) */
	static constexpr int32 BUTTONTIME = 35;

	// =========================================================================
	// Level scrolling (animated textures, scrolling walls)
	// =========================================================================

	/** Line indices with scrolling effect active */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	TArray<int32> ScrollingLines;

	/** Level timer counter (for animated flat cycling) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Specials")
	int32 LevelTimer = 0;

private:
	// =========================================================================
	// Per-tic update helpers
	// =========================================================================

	/** Update all active door thinkers */
	void UpdateDoors();

	/** Update all active floor movers */
	void UpdateFloors();

	/** Update all active ceiling movers */
	void UpdateCeilings();

	/** Update all active platforms */
	void UpdatePlatforms();

	/** Update all active light effects */
	void UpdateLights();

	/** Update button timers and revert switch textures */
	void UpdateButtons();

	/** Update animated textures and scrolling walls */
	void UpdateAnimations();

	// =========================================================================
	// References
	// =========================================================================

	/** Weak reference to the owning game mode */
	UPROPERTY()
	TWeakObjectPtr<ADoomGameMode> OwnerGameMode;
};
