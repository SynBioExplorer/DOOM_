// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.

#include "DoomPlayerController.h"
#include "DoomPlayerPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Core/DoomTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomInput, Log, All);

// =============================================================================
// Constructor
// =============================================================================

ADoomPlayerController::ADoomPlayerController()
{
	// Disable default UE camera management - DOOM handles its own view
	bAutoManageActiveCameraTarget = false;
}

// =============================================================================
// APlayerController overrides
// =============================================================================

void ADoomPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add the DOOM input mapping context to the Enhanced Input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DoomInputMappingContext)
		{
			Subsystem->AddMappingContext(DoomInputMappingContext, 0);
			UE_LOG(LogDoomInput, Log, TEXT("DOOM input mapping context added"));
		}
		else
		{
			UE_LOG(LogDoomInput, Warning, TEXT("No DOOM input mapping context assigned"));
		}
	}

	// Show the mouse cursor for menu interaction, hide for gameplay
	bShowMouseCursor = false;

	// Lock mouse to viewport for FPS gameplay
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ADoomPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogDoomInput, Error, TEXT("Failed to get EnhancedInputComponent"));
		return;
	}

	UE_LOG(LogDoomInput, Log, TEXT("Setting up DOOM input bindings"));

	// Movement: Forward/Back (Axis1D - W/S, Up/Down)
	if (IA_MoveForward)
	{
		EnhancedInput->BindAction(IA_MoveForward, ETriggerEvent::Triggered, this, &ADoomPlayerController::HandleMoveForward);
		EnhancedInput->BindAction(IA_MoveForward, ETriggerEvent::Completed, this, &ADoomPlayerController::HandleMoveForward);
	}

	// Movement: Strafe (Axis1D - A/D)
	if (IA_MoveStrafe)
	{
		EnhancedInput->BindAction(IA_MoveStrafe, ETriggerEvent::Triggered, this, &ADoomPlayerController::HandleMoveStrafe);
		EnhancedInput->BindAction(IA_MoveStrafe, ETriggerEvent::Completed, this, &ADoomPlayerController::HandleMoveStrafe);
	}

	// Keyboard turning (Axis1D - Left/Right arrows)
	if (IA_Turn)
	{
		EnhancedInput->BindAction(IA_Turn, ETriggerEvent::Triggered, this, &ADoomPlayerController::HandleTurn);
		EnhancedInput->BindAction(IA_Turn, ETriggerEvent::Completed, this, &ADoomPlayerController::HandleTurn);
	}

	// Mouse look (Axis2D - mouse delta X for turning)
	if (IA_MouseLook)
	{
		EnhancedInput->BindAction(IA_MouseLook, ETriggerEvent::Triggered, this, &ADoomPlayerController::HandleMouseLook);
	}

	// Fire weapon (Bool - Left mouse / Ctrl)
	if (IA_Fire)
	{
		EnhancedInput->BindAction(IA_Fire, ETriggerEvent::Started, this, &ADoomPlayerController::HandleFireStarted);
		EnhancedInput->BindAction(IA_Fire, ETriggerEvent::Completed, this, &ADoomPlayerController::HandleFireCompleted);
	}

	// Use / Activate (Bool - Spacebar / E)
	if (IA_Use)
	{
		EnhancedInput->BindAction(IA_Use, ETriggerEvent::Started, this, &ADoomPlayerController::HandleUseStarted);
		EnhancedInput->BindAction(IA_Use, ETriggerEvent::Completed, this, &ADoomPlayerController::HandleUseCompleted);
	}

	// Run modifier (Bool - Shift)
	if (IA_Run)
	{
		EnhancedInput->BindAction(IA_Run, ETriggerEvent::Started, this, &ADoomPlayerController::HandleRunStarted);
		EnhancedInput->BindAction(IA_Run, ETriggerEvent::Completed, this, &ADoomPlayerController::HandleRunCompleted);
	}

	// Automap toggle (Bool - Tab)
	if (IA_Automap)
	{
		EnhancedInput->BindAction(IA_Automap, ETriggerEvent::Started, this, &ADoomPlayerController::HandleAutomapToggle);
	}

	// Menu / Escape (Bool - Escape)
	if (IA_Menu)
	{
		EnhancedInput->BindAction(IA_Menu, ETriggerEvent::Started, this, &ADoomPlayerController::HandleMenuToggle);
	}

	// Weapon selection (1-7 keys)
	if (IA_Weapon1) { EnhancedInput->BindAction(IA_Weapon1, ETriggerEvent::Started, this, [this](const FInputActionValue&) { HandleWeaponSelect(1); }); }
	if (IA_Weapon2) { EnhancedInput->BindAction(IA_Weapon2, ETriggerEvent::Started, this, [this](const FInputActionValue&) { HandleWeaponSelect(2); }); }
	if (IA_Weapon3) { EnhancedInput->BindAction(IA_Weapon3, ETriggerEvent::Started, this, [this](const FInputActionValue&) { HandleWeaponSelect(3); }); }
	if (IA_Weapon4) { EnhancedInput->BindAction(IA_Weapon4, ETriggerEvent::Started, this, [this](const FInputActionValue&) { HandleWeaponSelect(4); }); }
	if (IA_Weapon5) { EnhancedInput->BindAction(IA_Weapon5, ETriggerEvent::Started, this, [this](const FInputActionValue&) { HandleWeaponSelect(5); }); }
	if (IA_Weapon6) { EnhancedInput->BindAction(IA_Weapon6, ETriggerEvent::Started, this, [this](const FInputActionValue&) { HandleWeaponSelect(6); }); }
	if (IA_Weapon7) { EnhancedInput->BindAction(IA_Weapon7, ETriggerEvent::Started, this, [this](const FInputActionValue&) { HandleWeaponSelect(7); }); }
}

void ADoomPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// The tic command is built on demand via BuildTicCmd() called from the game mode.
	// Input handlers accumulate state continuously between tics.
}

// =============================================================================
// Tic command building
// =============================================================================

FDoomTicCmd ADoomPlayerController::BuildTicCmd()
{
	// Start fresh each tic (equivalent to memset in G_BuildTiccmd)
	CurrentTicCmd.Clear();

	// Determine movement speed based on run state
	const bool bIsRunning = bRunHeld || bAlwaysRun;
	const int32 MaxMove = bIsRunning ? MAXPLMOVE_RUN : MAXPLMOVE_WALK;

	// Forward/backward movement
	CurrentTicCmd.ForwardMove = static_cast<int8>(FMath::Clamp(
		FMath::RoundToInt(AccumulatedForward * MaxMove), -MaxMove, MaxMove));

	// Strafe movement
	CurrentTicCmd.SideMove = static_cast<int8>(FMath::Clamp(
		FMath::RoundToInt(AccumulatedStrafe * MaxMove), -MaxMove, MaxMove));

	// Turning: combine keyboard and mouse input
	// Keyboard turning with acceleration (slow start, then fast)
	int32 TurnAmount = 0;

	if (FMath::Abs(AccumulatedTurn) > 0.1f)
	{
		TurnHeld++;
		const int32 TurnSpeed = (TurnHeld >= SLOWTURNTICS) ? ANGLETURN_FAST : ANGLETURN_SLOW;

		if (bIsRunning)
		{
			TurnAmount = FMath::RoundToInt(AccumulatedTurn * TurnSpeed * 2);
		}
		else
		{
			TurnAmount = FMath::RoundToInt(AccumulatedTurn * TurnSpeed);
		}
	}
	else
	{
		TurnHeld = 0;
	}

	// Add mouse contribution to turning
	// Mouse sensitivity is applied here (equivalent to mousex * mouseSensitivity in G_BuildTiccmd)
	TurnAmount += FMath::RoundToInt(AccumulatedMouseX * MouseSensitivityX * 8.0f);

	CurrentTicCmd.AngleTurn = static_cast<int16>(FMath::Clamp(TurnAmount, -32768, 32767));

	// Button flags
	if (bFireHeld)
	{
		CurrentTicCmd.Buttons |= BT_ATTACK;
	}

	if (bUseHeld)
	{
		CurrentTicCmd.Buttons |= BT_USE;
	}

	// Weapon change
	if (PendingWeaponSlot >= 0)
	{
		CurrentTicCmd.Buttons |= BT_CHANGE;
		CurrentTicCmd.Buttons |= static_cast<uint8>((PendingWeaponSlot << BT_WEAPONSHIFT) & BT_WEAPONMASK);
		PendingWeaponSlot = -1;
	}

	// Reset mouse accumulation (mouse is delta-based, consumed each tic)
	AccumulatedMouseX = 0.0f;

	// Reset per-tic flags
	bAutomapPressed = false;
	bMenuPressed = false;

	return CurrentTicCmd;
}

// =============================================================================
// Input handler callbacks
// =============================================================================

void ADoomPlayerController::HandleMoveForward(const FInputActionValue& Value)
{
	AccumulatedForward = Value.Get<float>();
}

void ADoomPlayerController::HandleMoveStrafe(const FInputActionValue& Value)
{
	AccumulatedStrafe = Value.Get<float>();
}

void ADoomPlayerController::HandleTurn(const FInputActionValue& Value)
{
	AccumulatedTurn = Value.Get<float>();
}

void ADoomPlayerController::HandleMouseLook(const FInputActionValue& Value)
{
	const FVector2D MouseDelta = Value.Get<FVector2D>();
	// Accumulate mouse X for turning; Y is unused in classic DOOM
	AccumulatedMouseX += MouseDelta.X;
}

void ADoomPlayerController::HandleFireStarted(const FInputActionValue& Value)
{
	bFireHeld = true;
}

void ADoomPlayerController::HandleFireCompleted(const FInputActionValue& Value)
{
	bFireHeld = false;
}

void ADoomPlayerController::HandleUseStarted(const FInputActionValue& Value)
{
	bUseHeld = true;
}

void ADoomPlayerController::HandleUseCompleted(const FInputActionValue& Value)
{
	bUseHeld = false;
}

void ADoomPlayerController::HandleRunStarted(const FInputActionValue& Value)
{
	bRunHeld = true;
}

void ADoomPlayerController::HandleRunCompleted(const FInputActionValue& Value)
{
	bRunHeld = false;
}

void ADoomPlayerController::HandleAutomapToggle(const FInputActionValue& Value)
{
	bAutomapPressed = true;
	UE_LOG(LogDoomInput, Verbose, TEXT("Automap toggle pressed"));
}

void ADoomPlayerController::HandleMenuToggle(const FInputActionValue& Value)
{
	bMenuPressed = true;
	UE_LOG(LogDoomInput, Verbose, TEXT("Menu toggle pressed"));
}

void ADoomPlayerController::HandleWeaponSelect(int32 WeaponSlot)
{
	// DOOM weapon slots: 1=Fist/Chainsaw, 2=Pistol, 3=Shotgun/SSG,
	// 4=Chaingun, 5=Rocket, 6=Plasma, 7=BFG
	if (WeaponSlot >= 1 && WeaponSlot <= 7)
	{
		PendingWeaponSlot = WeaponSlot - 1; // Convert to 0-based for BT_WEAPONMASK
		UE_LOG(LogDoomInput, Verbose, TEXT("Weapon slot %d selected"), WeaponSlot);
	}
}
