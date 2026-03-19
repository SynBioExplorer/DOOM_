#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomInputComponent - Manages DOOM input state and builds tic commands
// from UE5 Enhanced Input actions. Translates modern input (WASD, mouse,
// gamepad) into the classic DOOM ticcmd_t format with original movement speeds.
//
// Equivalent to G_BuildTiccmd() from g_game.c.

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "GameLogic/DoomTicCmd.h"
#include "DoomInputComponent.generated.h"

class UInputAction;
class UEnhancedInputComponent;

/**
 * UDoomInputComponent
 *
 * Captures player input via UE5 Enhanced Input and converts it into
 * DOOM tic commands (FDoomTicCmd). Attached to the player pawn or controller,
 * this component accumulates input state each frame and produces a tic command
 * at the fixed 35 Hz DOOM tick rate.
 *
 * Movement speeds match the original DOOM values:
 * - ForwardMove: 25 (walk), 50 (run)
 * - SideMove: 24 (walk), 40 (run)
 * - AngleTurn: 640 (normal), 1280 (fast turn), 320 (slow turn / strafe)
 *
 * Mouse look is accumulated per-frame and consumed each tic to prevent
 * input loss between frames and tics.
 */
UCLASS(ClassGroup = "Doom", meta = (BlueprintSpawnableComponent))
class UNREALDOOM_API UDoomInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoomInputComponent();

	// =========================================================================
	// UActorComponent overrides
	// =========================================================================

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =========================================================================
	// Input setup
	// =========================================================================

	/**
	 * Bind all DOOM input actions to an Enhanced Input component.
	 * Call this from the owning pawn's SetupPlayerInputComponent().
	 *
	 * @param InputComponent - The Enhanced Input component to bind to
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Input")
	void SetupInputBindings(UEnhancedInputComponent* InputComponent);

	// =========================================================================
	// Tic command generation
	// =========================================================================

	/**
	 * Build a DOOM tic command from the current accumulated input state.
	 * This is called once per DOOM tic (35 Hz) by the game mode.
	 * Consumes mouse delta accumulators.
	 *
	 * @return The tic command for this tick
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Input")
	FDoomTicCmd BuildTicCmd();

	// =========================================================================
	// Movement state (set by input callbacks, read by BuildTicCmd)
	// =========================================================================

	/** Player is pressing forward */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bForward = false;

	/** Player is pressing backward */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bBackward = false;

	/** Player is pressing strafe left */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bStrafeLeft = false;

	/** Player is pressing strafe right */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bStrafeRight = false;

	/** Player is pressing turn left */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bTurnLeft = false;

	/** Player is pressing turn right */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bTurnRight = false;

	/** Player is pressing fire */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bFire = false;

	/** Player is pressing use (open doors, activate switches) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bUse = false;

	/** Player is pressing sprint/run */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bSprint = false;

	/** Weapon slot requested this tic. -1 = no change requested. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	int32 WeaponSlotRequested = -1;

	/** Whether the automap toggle was pressed this tic */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bAutoMapToggle = false;

	/** Whether pause was pressed this tic */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|State")
	bool bPauseToggle = false;

	// =========================================================================
	// Mouse delta accumulators (accumulated per-frame, consumed per-tic)
	// =========================================================================

	/** Accumulated mouse X delta since last tic (turning) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|Mouse")
	float MouseDeltaX = 0.0f;

	/** Accumulated mouse Y delta since last tic (unused in vanilla DOOM, available for mouselook) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Input|Mouse")
	float MouseDeltaY = 0.0f;

	// =========================================================================
	// Settings
	// =========================================================================

	/** Toggle: always run (inverts sprint behavior) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Settings")
	bool bAlwaysRun = false;

	/** Mouse sensitivity multiplier for turning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Settings", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float MouseSensitivity = 1.0f;

	// =========================================================================
	// Input Actions (assign in Blueprint or constructor)
	// =========================================================================

	/** Move input action (Axis2D: forward/backward + strafe left/right) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Move;

	/** Look input action (Axis2D: mouse X/Y delta) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Look;

	/** Fire weapon action (bool) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Fire;

	/** Use/activate action (bool) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Use;

	/** Sprint/run action (bool) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Sprint;

	/** Weapon slot selection actions (1-7) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSlot1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSlot2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSlot3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSlot4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSlot5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSlot6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSlot7;

	/** Toggle automap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_AutoMap;

	/** Pause game */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Pause;

protected:
	// =========================================================================
	// Input callbacks
	// =========================================================================

	/** Called when move input changes (WASD / left stick) */
	void OnMoveInput(const FInputActionValue& Value);

	/** Called when move input is released */
	void OnMoveInputCompleted(const FInputActionValue& Value);

	/** Called when look input changes (mouse delta / right stick) */
	void OnLookInput(const FInputActionValue& Value);

	/** Fire pressed/released */
	void OnFireStarted(const FInputActionValue& Value);
	void OnFireCompleted(const FInputActionValue& Value);

	/** Use pressed/released */
	void OnUseStarted(const FInputActionValue& Value);
	void OnUseCompleted(const FInputActionValue& Value);

	/** Sprint pressed/released */
	void OnSprintStarted(const FInputActionValue& Value);
	void OnSprintCompleted(const FInputActionValue& Value);

	/** Weapon slot selection */
	void OnWeaponSlot1(const FInputActionValue& Value);
	void OnWeaponSlot2(const FInputActionValue& Value);
	void OnWeaponSlot3(const FInputActionValue& Value);
	void OnWeaponSlot4(const FInputActionValue& Value);
	void OnWeaponSlot5(const FInputActionValue& Value);
	void OnWeaponSlot6(const FInputActionValue& Value);
	void OnWeaponSlot7(const FInputActionValue& Value);

	/** Automap toggle */
	void OnAutoMapToggle(const FInputActionValue& Value);

	/** Pause toggle */
	void OnPauseToggle(const FInputActionValue& Value);

	// =========================================================================
	// DOOM movement speed tables (from g_game.c)
	// =========================================================================

	/** Forward/backward speed: [0] = walk, [1] = run */
	static constexpr int32 ForwardMove[2] = { 25, 50 };

	/** Strafe speed: [0] = walk, [1] = run */
	static constexpr int32 SideMove[2] = { 24, 40 };

	/** Turn speed: [0] = normal, [1] = fast, [2] = slow (for strafe-on / mouse) */
	static constexpr int32 AngleTurn[3] = { 640, 1280, 320 };

	/** Number of consecutive tics the player has been turning (for acceleration) */
	int32 TurnHeld = 0;

	/** Threshold tics before turn speed increases */
	static constexpr int32 SLOWTURNTICS = 6;
};
