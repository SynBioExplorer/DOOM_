// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.

#include "DoomPlayerPawn.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/DoomMath.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomPawn, Log, All);

// =============================================================================
// Constructor
// =============================================================================

ADoomPlayerPawn::ADoomPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// Create the capsule collision component as root
	// DOOM player: radius 16, height 56 -> UE: radius 32, half-height 56
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(PLAYER_RADIUS, PLAYER_HEIGHT * 0.5f);
	CapsuleComp->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComp->SetCanEverAffectNavigation(false);
	// DOOM handles its own physics - disable UE simulation
	CapsuleComp->SetSimulatePhysics(false);
	CapsuleComp->SetEnableGravity(false);
	SetRootComponent(CapsuleComp);

	// Create the first-person camera at DOOM viewheight
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(CapsuleComp);
	// Position camera at viewheight relative to capsule bottom
	// Capsule center is at half-height, so offset = viewheight - halfheight
	const float CameraZOffset = PLAYER_VIEWHEIGHT - (PLAYER_HEIGHT * 0.5f);
	CameraComp->SetRelativeLocation(FVector(0.0f, 0.0f, CameraZOffset));
	CameraComp->bUsePawnControlRotation = false; // DOOM controls the view angle directly
	CameraComp->FieldOfView = 90.0f; // Classic DOOM FOV

	// Create the weapon viewmodel mesh (attached to camera for first-person effect)
	WeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComp"));
	WeaponMeshComp->SetupAttachment(CameraComp);
	WeaponMeshComp->SetRelativeLocation(FVector(20.0f, 0.0f, -15.0f));
	WeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComp->CastShadow = false;
	WeaponMeshComp->SetVisibility(false); // Hidden until a weapon mesh is assigned
}

// =============================================================================
// APawn overrides
// =============================================================================

void ADoomPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogDoomPawn, Log, TEXT("DoomPlayerPawn spawned at %s"), *GetActorLocation().ToString());
}

void ADoomPlayerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Check if player is moving (for view bob purposes)
	const bool bIsMoving = (FMath::Abs(MomX) > FRACUNIT / 4) || (FMath::Abs(MomY) > FRACUNIT / 4);

	// Update view bobbing
	if (bEnableViewBob)
	{
		UpdateViewBob(DeltaSeconds, bIsMoving);
	}

	// Update damage/bonus flash decay
	UpdateFlashEffects(DeltaSeconds);

	// Apply any interpolation for smooth rendering between DOOM tics.
	// The authoritative DOOM position is set by ApplyTicCmd at 35 Hz,
	// but we can smoothly interpolate the UE transform at render framerate.
	SyncTransformFromDoom();
}

// =============================================================================
// DOOM movement interface
// =============================================================================

void ADoomPlayerPawn::ApplyTicCmd(const FDoomTicCmd& TicCmd)
{
	// Apply turning first (angle update)
	// AngleTurn is in DOOM angle units, applied directly to the binary angle
	DoomAngle += static_cast<int32>(TicCmd.AngleTurn) << 16;

	// Calculate thrust from forward/side move.
	// This is equivalent to P_Thrust in p_user.c:
	// player->mo->momx += cmd->forwardmove * finecosine[angle]
	// player->mo->momy += cmd->forwardmove * finesine[angle]
	if (TicCmd.ForwardMove != 0)
	{
		const uint32 AngleIdx = (static_cast<uint32>(DoomAngle) >> ANGLETOFINESHIFT) & FINEMASK;
		MomX += TicCmd.ForwardMove * FRACUNIT / 32; // Simplified thrust
		MomY += TicCmd.ForwardMove * FRACUNIT / 32;
	}

	if (TicCmd.SideMove != 0)
	{
		// Strafe is thrust perpendicular to facing angle
		MomX += TicCmd.SideMove * FRACUNIT / 32;
		MomY += TicCmd.SideMove * FRACUNIT / 32;
	}

	// Apply momentum to position (simplified P_XYMovement)
	// Full collision detection would go through P_TryMove -> P_CheckPosition
	DoomX += MomX;
	DoomY += MomY;
	DoomZ += MomZ;

	// Apply friction (DOOM friction factor: 0xe800 / 0x10000 ~ 0.90625)
	constexpr fixed_t FRICTION = 0xe800;
	MomX = FDoomMath::FixedMul(MomX, FRICTION);
	MomY = FDoomMath::FixedMul(MomY, FRICTION);

	// Gravity / Z-movement
	if (DoomZ > FloorZ)
	{
		// Apply gravity when above floor
		MomZ -= FRACUNIT; // GRAVITY constant from DOOM
	}
	else
	{
		// On the floor - stop vertical movement
		DoomZ = FloorZ;
		MomZ = 0;
	}

	// Update view bob phase when moving
	if (FMath::Abs(TicCmd.ForwardMove) > 0 || FMath::Abs(TicCmd.SideMove) > 0)
	{
		BobPhase = (BobPhase + 1) & 63; // Wrap at 64 tics
	}

	// Sync the UE transform from the updated DOOM state
	SyncTransformFromDoom();
}

void ADoomPlayerPawn::SetDoomPosition(int32 X, int32 Y, int32 Z)
{
	DoomX = X;
	DoomY = Y;
	DoomZ = Z;
	MomX = 0;
	MomY = 0;
	MomZ = 0;
	SyncTransformFromDoom();
}

void ADoomPlayerPawn::SetDoomAngle(int32 AngleValue)
{
	DoomAngle = AngleValue;
	SyncTransformFromDoom();
}

// =============================================================================
// Visual effects
// =============================================================================

void ADoomPlayerPawn::ApplyDamageFlash(int32 DamageAmount)
{
	// Scale flash intensity by damage, capped at 1.0
	// Original DOOM: damagecount used for palette index, max ~100
	const float Intensity = FMath::Clamp(static_cast<float>(DamageAmount) / 100.0f, 0.1f, 1.0f);
	DamageFlashIntensity = FMath::Max(DamageFlashIntensity, Intensity);
	FlashColor = FLinearColor(1.0f, 0.0f, 0.0f, DamageFlashIntensity); // Red
}

void ADoomPlayerPawn::ApplyBonusFlash(int32 BonusCount)
{
	const float Intensity = FMath::Clamp(static_cast<float>(BonusCount) / 6.0f, 0.1f, 1.0f);
	BonusFlashIntensity = FMath::Max(BonusFlashIntensity, Intensity);
	FlashColor = FLinearColor(1.0f, 0.85f, 0.0f, BonusFlashIntensity); // Gold
}

// =============================================================================
// Internal helpers
// =============================================================================

void ADoomPlayerPawn::SyncTransformFromDoom()
{
	// Convert DOOM position to UE world position
	// Coordinate system: DOOM Y -> UE X, DOOM X -> UE Y, DOOM Z -> UE Z
	const FVector UEPosition = FDoomMath::DoomPosToUE(DoomX, DoomY, DoomZ);
	SetActorLocation(UEPosition);

	// Convert DOOM binary angle to UE rotation
	// DOOM angles: 0 = East, counter-clockwise. UE: 0 = forward (X), counter-clockwise.
	const float DegreesYaw = FDoomMath::AngleToFloat(static_cast<angle_t>(DoomAngle));
	SetActorRotation(FRotator(0.0f, DegreesYaw, 0.0f));

	// Update camera with view bob offset
	if (CameraComp)
	{
		const float BaseZ = PLAYER_VIEWHEIGHT - (PLAYER_HEIGHT * 0.5f);
		CameraComp->SetRelativeLocation(FVector(0.0f, 0.0f, BaseZ + ViewBobOffset));
	}
}

void ADoomPlayerPawn::UpdateViewBob(float DeltaSeconds, bool bIsMoving)
{
	if (bIsMoving)
	{
		// Sinusoidal bob using the bob phase counter
		// Original DOOM uses player->bob calculated from momentum
		const float BobFraction = static_cast<float>(BobPhase) / 64.0f;
		const float BobAngle = BobFraction * 2.0f * PI;
		ViewBobOffset = FMath::Sin(BobAngle) * VIEW_BOB_AMPLITUDE;
	}
	else
	{
		// Decay bob to zero when standing still
		ViewBobOffset = FMath::FInterpTo(ViewBobOffset, 0.0f, DeltaSeconds, 10.0f);
	}
}

void ADoomPlayerPawn::UpdateFlashEffects(float DeltaSeconds)
{
	// Decay damage flash
	if (DamageFlashIntensity > 0.0f)
	{
		DamageFlashIntensity = FMath::Max(0.0f, DamageFlashIntensity - FLASH_DECAY_RATE * DeltaSeconds);
		if (DamageFlashIntensity > BonusFlashIntensity)
		{
			FlashColor = FLinearColor(1.0f, 0.0f, 0.0f, DamageFlashIntensity);
		}
	}

	// Decay bonus flash
	if (BonusFlashIntensity > 0.0f)
	{
		BonusFlashIntensity = FMath::Max(0.0f, BonusFlashIntensity - FLASH_DECAY_RATE * DeltaSeconds);
		if (BonusFlashIntensity > DamageFlashIntensity)
		{
			FlashColor = FLinearColor(1.0f, 0.85f, 0.0f, BonusFlashIntensity);
		}
	}

	// Clear flash when both are zero
	if (DamageFlashIntensity <= 0.0f && BonusFlashIntensity <= 0.0f)
	{
		FlashColor = FLinearColor::Black;
	}
}
