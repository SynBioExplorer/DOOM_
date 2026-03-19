#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// ADoomPlayerController - Handles input translation from UE5 Enhanced Input
// into DOOM ticcmd_t button/movement fields.
// Equivalent to the input processing in g_game.c G_BuildTiccmd.

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Core/DoomTypes.h"
#include "DoomPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ADoomPlayerPawn;

/**
 * FDoomTicCmd - The command structure built each tic from player input.
 * Direct port of ticcmd_t from d_ticcmd.h.
 * Contains all input state for a single game tic.
 */
USTRUCT(BlueprintType)
struct FDoomTicCmd
{
	GENERATED_BODY()

	/** Forward/backward movement speed (-MAXPLMOVE to MAXPLMOVE) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input")
	int8 ForwardMove = 0;

	/** Strafe left/right movement speed (-MAXPLMOVE to MAXPLMOVE) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input")
	int8 SideMove = 0;

	/** Turning speed (angular velocity, in angle units per tic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input")
	int16 AngleTurn = 0;

	/** Consistency check for network play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input")
	int16 Consistency = 0;

	/** Button state flags (BT_ATTACK, BT_USE, BT_CHANGE, weapon bits) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input")
	uint8 Buttons = 0;

	/** Reset all fields to zero */
	void Clear()
	{
		ForwardMove = 0;
		SideMove = 0;
		AngleTurn = 0;
		Consistency = 0;
		Buttons = 0;
	}
};

/**
 * ADoomPlayerController - Translates UE5 Enhanced Input into DOOM tic commands.
 *
 * Uses the Enhanced Input system to bind all classic DOOM controls:
 * - WASD / Arrow keys for movement
 * - Mouse for turning and firing
 * - Number keys for weapon selection
 * - Spacebar/E for use
 * - Tab for automap
 * - Escape for menu
 *
 * Each tick builds a FDoomTicCmd that drives the player pawn through
 * the DOOM physics system rather than UE movement components.
 */
UCLASS(Blueprintable, ClassGroup = "Doom")
class UNREALDOOM_API ADoomPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADoomPlayerController();

	// =========================================================================
	// APlayerController overrides
	// =========================================================================

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	// =========================================================================
	// Tic command access
	// =========================================================================

	/**
	 * Build and return the current tic command from accumulated input.
	 * Called by the game mode each DOOM tic (35 Hz).
	 * Equivalent to G_BuildTiccmd.
	 *
	 * @return The tic command for this frame
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Input")
	FDoomTicCmd BuildTicCmd();

	/** @return The most recently built tic command */
	UFUNCTION(BlueprintPure, Category = "Doom|Input")
	const FDoomTicCmd& GetCurrentTicCmd() const { return CurrentTicCmd; }

	// =========================================================================
	// Settings
	// =========================================================================

	/** Mouse sensitivity multiplier for horizontal look */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Settings")
	float MouseSensitivityX = 5.0f;

	/** Mouse sensitivity multiplier for vertical look (unused in classic DOOM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Settings")
	float MouseSensitivityY = 0.0f;

	/** Whether to use classic DOOM keyboard turning speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Settings")
	bool bClassicTurning = true;

	/** Whether running is toggled or held */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Input|Settings")
	bool bAlwaysRun = false;

protected:
	// =========================================================================
	// Enhanced Input setup
	// =========================================================================

	/** Input mapping context with all DOOM bindings */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input")
	TObjectPtr<UInputMappingContext> DoomInputMappingContext;

	/** Input action: Move forward/backward (W/S, Up/Down arrows) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_MoveForward;

	/** Input action: Strafe left/right (A/D) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_MoveStrafe;

	/** Input action: Turn left/right (Left/Right arrows) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Turn;

	/** Input action: Mouse look (mouse X axis for turning) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_MouseLook;

	/** Input action: Fire weapon (Left mouse button, Ctrl) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Fire;

	/** Input action: Use / activate (Spacebar, E) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Use;

	/** Input action: Run modifier (Shift) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Run;

	/** Input action: Toggle automap (Tab) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Automap;

	/** Input action: Menu / escape */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Menu;

	/** Input actions: Weapon select (1-7 keys) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Weapon1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Weapon2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Weapon3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Weapon4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Weapon5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Weapon6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Doom|Input|Actions")
	TObjectPtr<UInputAction> IA_Weapon7;

	// =========================================================================
	// Input handler callbacks
	// =========================================================================

	void HandleMoveForward(const FInputActionValue& Value);
	void HandleMoveStrafe(const FInputActionValue& Value);
	void HandleTurn(const FInputActionValue& Value);
	void HandleMouseLook(const FInputActionValue& Value);
	void HandleFireStarted(const FInputActionValue& Value);
	void HandleFireCompleted(const FInputActionValue& Value);
	void HandleUseStarted(const FInputActionValue& Value);
	void HandleUseCompleted(const FInputActionValue& Value);
	void HandleRunStarted(const FInputActionValue& Value);
	void HandleRunCompleted(const FInputActionValue& Value);
	void HandleAutomapToggle(const FInputActionValue& Value);
	void HandleMenuToggle(const FInputActionValue& Value);
	void HandleWeaponSelect(int32 WeaponSlot);

	// =========================================================================
	// Input state accumulation
	// =========================================================================

	/** Accumulated forward movement this frame (-1.0 to 1.0) */
	float AccumulatedForward = 0.0f;

	/** Accumulated strafe movement this frame (-1.0 to 1.0) */
	float AccumulatedStrafe = 0.0f;

	/** Accumulated turn amount this frame (from keyboard) */
	float AccumulatedTurn = 0.0f;

	/** Accumulated mouse X delta this frame */
	float AccumulatedMouseX = 0.0f;

	/** Whether the fire button is currently held */
	bool bFireHeld = false;

	/** Whether the use button is currently held */
	bool bUseHeld = false;

	/** Whether the run modifier is currently held */
	bool bRunHeld = false;

	/** Pending weapon change slot (-1 = no change pending) */
	int32 PendingWeaponSlot = -1;

	/** Whether automap toggle was pressed this tic */
	bool bAutomapPressed = false;

	/** Whether menu toggle was pressed this tic */
	bool bMenuPressed = false;

	/** The current built tic command */
	FDoomTicCmd CurrentTicCmd;

	// =========================================================================
	// DOOM movement constants (from g_game.c)
	// =========================================================================

	/** Maximum walking movement speed per tic */
	static constexpr int32 MAXPLMOVE_WALK = 25;

	/** Maximum running movement speed per tic */
	static constexpr int32 MAXPLMOVE_RUN = 50;

	/** Keyboard turning speed (slow) */
	static constexpr int32 SLOWTURNTICS = 6;

	/** Keyboard turning speed values (slow, fast) */
	static constexpr int32 ANGLETURN_SLOW = 640;
	static constexpr int32 ANGLETURN_FAST = 1280;

	/** Number of consecutive tics turning (for acceleration) */
	int32 TurnHeld = 0;
};
