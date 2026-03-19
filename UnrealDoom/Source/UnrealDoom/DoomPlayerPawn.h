#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// ADoomPlayerPawn - First-person pawn whose movement is driven entirely
// by the DOOM physics engine via tic commands, not by UE character movement.
// Equivalent to the player mobj_t and associated rendering in r_main.c.

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/DoomTypes.h"
#include "DoomPlayerController.h"
#include "DoomPlayerPawn.generated.h"

class UCapsuleComponent;
class UCameraComponent;
class UStaticMeshComponent;

/**
 * ADoomPlayerPawn - The player's physical representation in the DOOM world.
 *
 * Uses a capsule collider sized to match the original DOOM player:
 * - DOOM radius: 16 units -> 32 UE units (x2 scale)
 * - DOOM height: 56 units -> 112 UE units (x2 scale)
 * - DOOM viewheight: 41 units -> 82 UE units (x2 scale)
 *
 * Movement is NOT handled by UCharacterMovementComponent.
 * Instead, the DOOM physics system (P_MovePlayer, P_XYMovement, P_ZMovement)
 * calculates position updates each tic, and this pawn applies them.
 *
 * Visual effects include view bobbing and damage flash, matching the
 * original DOOM renderer behavior.
 */
UCLASS(Blueprintable, ClassGroup = "Doom")
class UNREALDOOM_API ADoomPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	ADoomPlayerPawn();

	// =========================================================================
	// APawn overrides
	// =========================================================================

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// =========================================================================
	// DOOM position / movement interface
	// =========================================================================

	/**
	 * Apply a tic command to update the pawn's DOOM-space position and angle.
	 * Called by the game mode at 35 Hz. This is the main movement entry point.
	 *
	 * @param TicCmd - The input command for this tic
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Movement")
	void ApplyTicCmd(const FDoomTicCmd& TicCmd);

	/**
	 * Set the pawn's position in DOOM fixed-point coordinates.
	 * Converts and applies to the UE actor transform.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Movement")
	void SetDoomPosition(int32 X, int32 Y, int32 Z);

	/**
	 * Set the pawn's facing angle in DOOM binary angle format.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Movement")
	void SetDoomAngle(int32 AngleValue);

	/** @return Current DOOM X position (fixed-point) */
	UFUNCTION(BlueprintPure, Category = "Doom|Movement")
	int32 GetDoomX() const { return DoomX; }

	/** @return Current DOOM Y position (fixed-point) */
	UFUNCTION(BlueprintPure, Category = "Doom|Movement")
	int32 GetDoomY() const { return DoomY; }

	/** @return Current DOOM Z position (fixed-point) */
	UFUNCTION(BlueprintPure, Category = "Doom|Movement")
	int32 GetDoomZ() const { return DoomZ; }

	/** @return Current DOOM facing angle (binary angle) */
	UFUNCTION(BlueprintPure, Category = "Doom|Movement")
	int32 GetDoomAngle() const { return DoomAngle; }

	// =========================================================================
	// Visual effects
	// =========================================================================

	/**
	 * Apply a damage flash effect (screen turns red briefly).
	 * @param DamageAmount - Damage taken, controls flash intensity
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Effects")
	void ApplyDamageFlash(int32 DamageAmount);

	/**
	 * Apply a pickup flash effect (screen turns gold/green briefly).
	 * @param BonusCount - Bonus flash intensity
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Effects")
	void ApplyBonusFlash(int32 BonusCount);

	// =========================================================================
	// Components
	// =========================================================================

	/** @return The capsule collision component */
	UFUNCTION(BlueprintPure, Category = "Doom|Components")
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComp; }

	/** @return The first-person camera component */
	UFUNCTION(BlueprintPure, Category = "Doom|Components")
	UCameraComponent* GetCameraComponent() const { return CameraComp; }

	/** @return The weapon viewmodel mesh component */
	UFUNCTION(BlueprintPure, Category = "Doom|Components")
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMeshComp; }

protected:
	// =========================================================================
	// Components
	// =========================================================================

	/** Capsule collision root - sized to DOOM player dimensions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Components")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	/** First-person camera at DOOM viewheight */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Components")
	TObjectPtr<UCameraComponent> CameraComp;

	/** Weapon viewmodel display (attached to camera) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Components")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComp;

	// =========================================================================
	// DOOM position state (fixed-point, authoritative)
	// =========================================================================

	/** DOOM X position in fixed-point (16.16) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 DoomX = 0;

	/** DOOM Y position in fixed-point (16.16) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 DoomY = 0;

	/** DOOM Z position (floor-relative) in fixed-point */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 DoomZ = 0;

	/** DOOM facing angle (binary angle measurement) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 DoomAngle = 0;

	/** DOOM momentum X in fixed-point (applied each tic by P_XYMovement) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 MomX = 0;

	/** DOOM momentum Y in fixed-point */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 MomY = 0;

	/** DOOM momentum Z in fixed-point (gravity, jumping if modded) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 MomZ = 0;

	/** Floor height at current position (fixed-point) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 FloorZ = 0;

	/** Ceiling height at current position (fixed-point) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|State")
	int32 CeilingZ = 0;

	// =========================================================================
	// View bobbing
	// =========================================================================

	/** View bob phase (0-based tic counter, wraps at 64) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|View")
	int32 BobPhase = 0;

	/** Current view bob offset in UE units */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|View")
	float ViewBobOffset = 0.0f;

	/** Whether view bobbing is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|View")
	bool bEnableViewBob = true;

	// =========================================================================
	// Damage / bonus flash
	// =========================================================================

	/** Current damage flash intensity (0.0 to 1.0, decays over time) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Effects")
	float DamageFlashIntensity = 0.0f;

	/** Current bonus flash intensity */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Effects")
	float BonusFlashIntensity = 0.0f;

	/** Color for the current flash overlay */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Effects")
	FLinearColor FlashColor = FLinearColor::Black;

	// =========================================================================
	// Dimension constants (DOOM units * 2 = UE units)
	// =========================================================================

	/** DOOM player radius: 16 units -> 32 UE units */
	static constexpr float PLAYER_RADIUS = 16.0f * 2.0f;

	/** DOOM player height: 56 units -> 112 UE units */
	static constexpr float PLAYER_HEIGHT = 56.0f * 2.0f;

	/** DOOM player view height: 41 units -> 82 UE units */
	static constexpr float PLAYER_VIEWHEIGHT = 41.0f * 2.0f;

	/** View bob amplitude in UE units */
	static constexpr float VIEW_BOB_AMPLITUDE = 8.0f;

	/** Damage flash decay rate per second */
	static constexpr float FLASH_DECAY_RATE = 4.0f;

private:
	/** Sync the UE actor transform from DOOM position/angle state. */
	void SyncTransformFromDoom();

	/** Calculate view bobbing offset based on movement. */
	void UpdateViewBob(float DeltaSeconds, bool bIsMoving);

	/** Update damage/bonus flash decay. */
	void UpdateFlashEffects(float DeltaSeconds);
};
