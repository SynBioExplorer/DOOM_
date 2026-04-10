#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_map.c - Movement, collision, and line attacks.

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/DoomTypes.h"
#include "DoomCollision.generated.h"

class AActor;
class UWorld;

// =============================================================================
// FDoomLineAttackResult - Output of a line attack (hitscan)
// =============================================================================

USTRUCT(BlueprintType)
struct FDoomLineAttackResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool bHit = false;

	UPROPERTY(BlueprintReadWrite)
	AActor* HitActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	FVector HitNormal = FVector::ZeroVector;

	/** Slope used for the shot (DOOM fixed-point) */
	UPROPERTY(BlueprintReadWrite)
	int32 Slope = 0;
};

// =============================================================================
// FDoomMoveResult - Output of a P_TryMove attempt
// =============================================================================

USTRUCT(BlueprintType)
struct FDoomMoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool bSuccess = false;

	/** True if the move was valid but blocked by step height/ceiling */
	UPROPERTY(BlueprintReadWrite)
	bool bFloatOk = false;

	/** New floor Z the thing would rest on */
	UPROPERTY(BlueprintReadWrite)
	fixed_t FloorZ = 0;

	/** New ceiling Z */
	UPROPERTY(BlueprintReadWrite)
	fixed_t CeilingZ = 0;

	/** Z of step-down edge (for drop-off checks) */
	UPROPERTY(BlueprintReadWrite)
	fixed_t DropOffZ = 0;
};

// =============================================================================
// UDoomCollision - Static collision and movement functions
// =============================================================================

UCLASS(BlueprintType)
class UNREALDOOM_API UDoomCollision : public UObject
{
	GENERATED_BODY()

public:
	// =========================================================================
	// Core collision functions (ported from p_map.c)
	// =========================================================================

	/**
	 * P_CheckPosition: Check if a thing can fit at (x,y) without colliding.
	 * Does NOT actually move the thing - query only.
	 * @param Thing Actor to test
	 * @param X Target X in DOOM fixed-point
	 * @param Y Target Y in DOOM fixed-point
	 * @param OutResult Filled with floor/ceiling info if valid
	 * @return true if position is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static bool CheckPosition(AActor* Thing, fixed_t X, fixed_t Y, FDoomMoveResult& OutResult);

	/**
	 * P_TryMove: Attempt to move a thing to (x,y). Updates position if successful.
	 * Handles step-up, drop-off, and line special activation.
	 * @param Thing Actor to move
	 * @param X Target X in DOOM fixed-point
	 * @param Y Target Y in DOOM fixed-point
	 * @return true if the move succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static bool TryMove(AActor* Thing, fixed_t X, fixed_t Y);

	/**
	 * P_TeleportMove: Force a thing to a new position, telefragging any blockers.
	 * Used by teleporters and player spawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static bool TeleportMove(AActor* Thing, fixed_t X, fixed_t Y);

	// =========================================================================
	// Line attacks (hitscan)
	// =========================================================================

	/**
	 * P_AimLineAttack: Find a target along a line within a range.
	 * Auto-aim for player weapons.
	 * @return Slope to use for the shot
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static int32 AimLineAttack(AActor* Shooter, angle_t Angle, fixed_t Distance,
		AActor*& OutLineTarget);

	/**
	 * P_LineAttack: Hitscan attack with damage.
	 * Spawns bullet puffs, damages targets.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static FDoomLineAttackResult LineAttack(AActor* Shooter, angle_t Angle,
		fixed_t Distance, int32 Slope, int32 Damage);

	// =========================================================================
	// Radius damage
	// =========================================================================

	/**
	 * P_RadiusAttack: Splash damage in a radius.
	 * Used by rockets, BFG ball, barrels, archvile fire.
	 * @param Spot Center of explosion
	 * @param Source Who gets credit for kills (can be null)
	 * @param Damage Full damage at distance 0
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static void RadiusAttack(AActor* Spot, AActor* Source, int32 Damage);

	// =========================================================================
	// Thing Z/position sync
	// =========================================================================

	/** P_ThingHeightClip: Called when a sector moves to resync a thing's Z */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static bool ThingHeightClip(AActor* Thing);

	/** P_SlideMove: Slide along walls when blocked (player movement) */
	UFUNCTION(BlueprintCallable, Category = "Doom Collision")
	static void SlideMove(AActor* Mo);

	// =========================================================================
	// Utility
	// =========================================================================

	/** Convert DOOM fixed-point to UE5 world units (and vice versa) */
	static float FixedToUE(fixed_t Value) { return static_cast<float>(Value) / FRACUNIT; }
	static fixed_t UEToFixed(float Value) { return static_cast<fixed_t>(Value * FRACUNIT); }

	/** Distance constants from DOOM */
	static constexpr fixed_t MELEE_RANGE = 64 * FRACUNIT;
	static constexpr fixed_t MISSILE_RANGE = 32 * 64 * FRACUNIT;
	static constexpr fixed_t MAX_RADIUS = 32 * FRACUNIT;
	static constexpr fixed_t MAX_STEP = 24 * FRACUNIT;
	static constexpr fixed_t GRAVITY = FRACUNIT;
};
