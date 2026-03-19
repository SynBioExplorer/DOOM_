// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomInputComponent implementation - builds DOOM tic commands from
// UE5 Enhanced Input. Mirrors G_BuildTiccmd() from g_game.c.

#include "DoomInputComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Core/DoomTypes.h"

UDoomInputComponent::UDoomInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDoomInputComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDoomInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Track how long the player has been holding a turn key (for turn acceleration)
	if (bTurnLeft || bTurnRight)
	{
		TurnHeld++;
	}
	else
	{
		TurnHeld = 0;
	}
}

// =============================================================================
// Input binding setup
// =============================================================================

void UDoomInputComponent::SetupInputBindings(UEnhancedInputComponent* InputComponent)
{
	if (!InputComponent)
	{
		return;
	}

	// Movement (Axis2D)
	if (IA_Move)
	{
		InputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &UDoomInputComponent::OnMoveInput);
		InputComponent->BindAction(IA_Move, ETriggerEvent::Completed, this, &UDoomInputComponent::OnMoveInputCompleted);
	}

	// Look (Axis2D - mouse delta)
	if (IA_Look)
	{
		InputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &UDoomInputComponent::OnLookInput);
	}

	// Fire
	if (IA_Fire)
	{
		InputComponent->BindAction(IA_Fire, ETriggerEvent::Started, this, &UDoomInputComponent::OnFireStarted);
		InputComponent->BindAction(IA_Fire, ETriggerEvent::Completed, this, &UDoomInputComponent::OnFireCompleted);
	}

	// Use
	if (IA_Use)
	{
		InputComponent->BindAction(IA_Use, ETriggerEvent::Started, this, &UDoomInputComponent::OnUseStarted);
		InputComponent->BindAction(IA_Use, ETriggerEvent::Completed, this, &UDoomInputComponent::OnUseCompleted);
	}

	// Sprint
	if (IA_Sprint)
	{
		InputComponent->BindAction(IA_Sprint, ETriggerEvent::Started, this, &UDoomInputComponent::OnSprintStarted);
		InputComponent->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &UDoomInputComponent::OnSprintCompleted);
	}

	// Weapon slots 1-7
	if (IA_WeaponSlot1)
	{
		InputComponent->BindAction(IA_WeaponSlot1, ETriggerEvent::Started, this, &UDoomInputComponent::OnWeaponSlot1);
	}
	if (IA_WeaponSlot2)
	{
		InputComponent->BindAction(IA_WeaponSlot2, ETriggerEvent::Started, this, &UDoomInputComponent::OnWeaponSlot2);
	}
	if (IA_WeaponSlot3)
	{
		InputComponent->BindAction(IA_WeaponSlot3, ETriggerEvent::Started, this, &UDoomInputComponent::OnWeaponSlot3);
	}
	if (IA_WeaponSlot4)
	{
		InputComponent->BindAction(IA_WeaponSlot4, ETriggerEvent::Started, this, &UDoomInputComponent::OnWeaponSlot4);
	}
	if (IA_WeaponSlot5)
	{
		InputComponent->BindAction(IA_WeaponSlot5, ETriggerEvent::Started, this, &UDoomInputComponent::OnWeaponSlot5);
	}
	if (IA_WeaponSlot6)
	{
		InputComponent->BindAction(IA_WeaponSlot6, ETriggerEvent::Started, this, &UDoomInputComponent::OnWeaponSlot6);
	}
	if (IA_WeaponSlot7)
	{
		InputComponent->BindAction(IA_WeaponSlot7, ETriggerEvent::Started, this, &UDoomInputComponent::OnWeaponSlot7);
	}

	// Automap
	if (IA_AutoMap)
	{
		InputComponent->BindAction(IA_AutoMap, ETriggerEvent::Started, this, &UDoomInputComponent::OnAutoMapToggle);
	}

	// Pause
	if (IA_Pause)
	{
		InputComponent->BindAction(IA_Pause, ETriggerEvent::Started, this, &UDoomInputComponent::OnPauseToggle);
	}
}

// =============================================================================
// BuildTicCmd - Core tic command generation (mirrors G_BuildTiccmd)
// =============================================================================

FDoomTicCmd UDoomInputComponent::BuildTicCmd()
{
	FDoomTicCmd Cmd;
	Cmd.Clear();

	// Determine if running: sprint XOR always-run
	const bool bRunning = bSprint != bAlwaysRun;
	const int32 SpeedIndex = bRunning ? 1 : 0;

	// -------------------------------------------------------------------------
	// Forward / backward movement
	// -------------------------------------------------------------------------

	if (bForward)
	{
		Cmd.ForwardMove += ForwardMove[SpeedIndex];
	}
	if (bBackward)
	{
		Cmd.ForwardMove -= ForwardMove[SpeedIndex];
	}

	// -------------------------------------------------------------------------
	// Strafe movement
	// -------------------------------------------------------------------------

	if (bStrafeRight)
	{
		Cmd.SideMove += SideMove[SpeedIndex];
	}
	if (bStrafeLeft)
	{
		Cmd.SideMove -= SideMove[SpeedIndex];
	}

	// -------------------------------------------------------------------------
	// Turning (keyboard)
	// -------------------------------------------------------------------------

	int32 TurnSpeed;
	if (bRunning)
	{
		// Running uses fast turn
		TurnSpeed = AngleTurn[1];
	}
	else if (TurnHeld < SLOWTURNTICS)
	{
		// Initial slow turn for precision
		TurnSpeed = AngleTurn[2];
	}
	else
	{
		// Normal turn speed after held
		TurnSpeed = AngleTurn[0];
	}

	if (bTurnRight)
	{
		Cmd.AngleTurn -= TurnSpeed;
	}
	if (bTurnLeft)
	{
		Cmd.AngleTurn += TurnSpeed;
	}

	// -------------------------------------------------------------------------
	// Mouse turning (accumulated delta, consumed here)
	// -------------------------------------------------------------------------

	if (FMath::Abs(MouseDeltaX) > KINDA_SMALL_NUMBER)
	{
		// Mouse turn: scale by sensitivity. Original DOOM used mousex * 8.
		Cmd.AngleTurn -= FMath::RoundToInt32(MouseDeltaX * 8.0f * MouseSensitivity);
		MouseDeltaX = 0.0f;
	}

	// Reset mouse Y accumulator (not used for vanilla DOOM vertical look)
	MouseDeltaY = 0.0f;

	// -------------------------------------------------------------------------
	// Clamp movement values to DOOM's int8 range
	// -------------------------------------------------------------------------

	Cmd.ForwardMove = FMath::Clamp(Cmd.ForwardMove, -127, 127);
	Cmd.SideMove = FMath::Clamp(Cmd.SideMove, -127, 127);

	// -------------------------------------------------------------------------
	// Buttons
	// -------------------------------------------------------------------------

	if (bFire)
	{
		Cmd.Buttons |= DoomButtons::BT_ATTACK;
	}

	if (bUse)
	{
		Cmd.Buttons |= DoomButtons::BT_USE;
		// Use is consumed on press (single trigger per press)
		bUse = false;
	}

	// -------------------------------------------------------------------------
	// Weapon change
	// -------------------------------------------------------------------------

	if (WeaponSlotRequested >= 0 && WeaponSlotRequested <= 7)
	{
		Cmd.Buttons |= DoomButtons::BT_CHANGE;
		Cmd.Buttons |= static_cast<uint8>((WeaponSlotRequested & 0x07) << DoomButtons::BT_WEAPONSHIFT);
		WeaponSlotRequested = -1;
	}

	// -------------------------------------------------------------------------
	// Special buttons (pause)
	// -------------------------------------------------------------------------

	if (bPauseToggle)
	{
		Cmd.Buttons |= DoomButtons::BT_SPECIAL;
		Cmd.Buttons |= DoomButtons::BTS_PAUSE;
		bPauseToggle = false;
	}

	return Cmd;
}

// =============================================================================
// Input callbacks
// =============================================================================

void UDoomInputComponent::OnMoveInput(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();

	// Y axis = forward/backward
	bForward = MoveVector.Y > 0.1f;
	bBackward = MoveVector.Y < -0.1f;

	// X axis = strafe left/right
	bStrafeRight = MoveVector.X > 0.1f;
	bStrafeLeft = MoveVector.X < -0.1f;
}

void UDoomInputComponent::OnMoveInputCompleted(const FInputActionValue& Value)
{
	bForward = false;
	bBackward = false;
	bStrafeLeft = false;
	bStrafeRight = false;
}

void UDoomInputComponent::OnLookInput(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	// Accumulate mouse delta - consumed by BuildTicCmd
	MouseDeltaX += LookVector.X;
	MouseDeltaY += LookVector.Y;
}

void UDoomInputComponent::OnFireStarted(const FInputActionValue& Value)
{
	bFire = true;
}

void UDoomInputComponent::OnFireCompleted(const FInputActionValue& Value)
{
	bFire = false;
}

void UDoomInputComponent::OnUseStarted(const FInputActionValue& Value)
{
	bUse = true;
}

void UDoomInputComponent::OnUseCompleted(const FInputActionValue& Value)
{
	bUse = false;
}

void UDoomInputComponent::OnSprintStarted(const FInputActionValue& Value)
{
	bSprint = true;
}

void UDoomInputComponent::OnSprintCompleted(const FInputActionValue& Value)
{
	bSprint = false;
}

void UDoomInputComponent::OnWeaponSlot1(const FInputActionValue& Value) { WeaponSlotRequested = 0; }
void UDoomInputComponent::OnWeaponSlot2(const FInputActionValue& Value) { WeaponSlotRequested = 1; }
void UDoomInputComponent::OnWeaponSlot3(const FInputActionValue& Value) { WeaponSlotRequested = 2; }
void UDoomInputComponent::OnWeaponSlot4(const FInputActionValue& Value) { WeaponSlotRequested = 3; }
void UDoomInputComponent::OnWeaponSlot5(const FInputActionValue& Value) { WeaponSlotRequested = 4; }
void UDoomInputComponent::OnWeaponSlot6(const FInputActionValue& Value) { WeaponSlotRequested = 5; }
void UDoomInputComponent::OnWeaponSlot7(const FInputActionValue& Value) { WeaponSlotRequested = 6; }

void UDoomInputComponent::OnAutoMapToggle(const FInputActionValue& Value)
{
	bAutoMapToggle = true;
}

void UDoomInputComponent::OnPauseToggle(const FInputActionValue& Value)
{
	bPauseToggle = true;
}
