// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 - ADoomEntity implementation

#include "DoomEntity.h"
#include "Engine/World.h"

// =============================================================================
// Constructor
// =============================================================================

ADoomEntity::ADoomEntity()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision sphere (root component)
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetSphereRadius(20.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(CollisionComponent);

	// Placeholder visual mesh
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Defaults (matching a null mobj_t)
	Type = EDoomMobjType::MT_PLAYER;
	Position = FVector::ZeroVector;
	Angle = 0.0f;
	MobjFlags = 0;
	Health = 1000;
	Momentum = FVector::ZeroVector;
	Radius = 20.0f;
	Height = 16.0f;
	FloorZ = 0.0f;
	CeilingZ = 0.0f;
	Sprite = 0;
	Frame = 0;
	Tics = -1;
	CurrentStateIndex = 0;
	ActionFunction = nullptr;
	MoveDir = EDoomDirection::NoDir;
	MoveCount = 0;
	ReactionTime = 0;
	Threshold = 0;
	Target = nullptr;
	Tracer = nullptr;
	LastLook = 0;
	SectorIndex = -1;
	SubsectorIndex = -1;
	SectorNext = nullptr;
	SectorPrev = nullptr;
	BlockNext = nullptr;
	BlockPrev = nullptr;
	Mass = 100;
	Damage = 0;
	Speed = 0;
	PlayerNumber = -1;
	TicAccumulator = 0.0f;
}

// =============================================================================
// BeginPlay
// =============================================================================

void ADoomEntity::BeginPlay()
{
	Super::BeginPlay();

	// Sync UE actor position with DOOM position
	SetActorLocation(Position);
	SetActorRotation(FRotator(0.0f, Angle, 0.0f));
}

// =============================================================================
// Tick - drives the DOOM 35Hz thinker loop
// =============================================================================

void ADoomEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TicAccumulator += DeltaTime;

	// Process DOOM tics at fixed 35Hz rate
	while (TicAccumulator >= DOOM_TIC_DURATION)
	{
		TicAccumulator -= DOOM_TIC_DURATION;
		MobjThinker(DOOM_TIC_DURATION);
	}

	// Interpolate UE actor position from DOOM position
	SetActorLocation(Position);
	SetActorRotation(FRotator(0.0f, Angle, 0.0f));
}

// =============================================================================
// P_MobjThinker port
// =============================================================================

void ADoomEntity::MobjThinker(float DeltaTime)
{
	// Apply momentum
	if (Momentum.X != 0.0f || Momentum.Y != 0.0f)
	{
		XYMovement();
	}

	if ((Position.Z != FloorZ) || Momentum.Z != 0.0f)
	{
		ZMovement();
	}

	// Advance state machine
	if (Tics != -1)
	{
		Tics--;

		if (Tics <= 0)
		{
			// State expired - transition to next state
			// In full implementation, this reads from the state table
			// For now, call action function if set
			if (ActionFunction)
			{
				ActionFunction(this);
			}
		}
	}
}

// =============================================================================
// P_XYMovement port - horizontal movement with friction
// =============================================================================

void ADoomEntity::XYMovement()
{
	// Apply momentum to position
	Position.X += Momentum.X;
	Position.Y += Momentum.Y;

	// Apply friction for non-missile, non-flying, on-ground entities
	if (!(MobjFlags & MF_MISSILE) && !(MobjFlags & MF_SKULLFLY))
	{
		if (FMath::Abs(Position.Z - FloorZ) < 1.0f)
		{
			ApplyFriction();
		}
	}

	// Missiles: if momentum is zero, remove (hit something)
	if (MobjFlags & MF_MISSILE)
	{
		if (Momentum.X == 0.0f && Momentum.Y == 0.0f)
		{
			// Missile has stopped - will be handled by collision system
		}
	}
}

// =============================================================================
// P_ZMovement port - vertical movement and gravity
// =============================================================================

void ADoomEntity::ZMovement()
{
	// Apply vertical momentum
	Position.Z += Momentum.Z;

	// Apply gravity
	if (!(MobjFlags & MF_NOGRAVITY))
	{
		ApplyGravity();
	}

	// Floor clipping
	ClipToFloorCeiling();
}

// =============================================================================
// ClipToFloorCeiling
// =============================================================================

void ADoomEntity::ClipToFloorCeiling()
{
	// Floor clip
	if (Position.Z <= FloorZ)
	{
		Position.Z = FloorZ;

		if (Momentum.Z < 0.0f)
		{
			Momentum.Z = 0.0f;
		}

		// Missiles explode on floor
		if ((MobjFlags & MF_MISSILE) && !(MobjFlags & MF_NOCLIP))
		{
			// ExplodeMissile() would be called here
		}
	}

	// Ceiling clip
	if (Position.Z + Height > CeilingZ)
	{
		Position.Z = CeilingZ - Height;

		if (Momentum.Z > 0.0f)
		{
			Momentum.Z = 0.0f;
		}

		// Missiles explode on ceiling
		if (MobjFlags & MF_MISSILE)
		{
			// ExplodeMissile() would be called here
		}
	}
}

// =============================================================================
// ApplyFriction
// =============================================================================

void ADoomEntity::ApplyFriction()
{
	// Only apply friction when on the ground
	if (FMath::Abs(Position.Z - FloorZ) < 1.0f)
	{
		Momentum.X *= DOOM_FRICTION;
		Momentum.Y *= DOOM_FRICTION;

		// Snap to zero if very small
		if (FMath::Abs(Momentum.X) < 0.01f) Momentum.X = 0.0f;
		if (FMath::Abs(Momentum.Y) < 0.01f) Momentum.Y = 0.0f;
	}
}

// =============================================================================
// ApplyGravity
// =============================================================================

void ADoomEntity::ApplyGravity()
{
	if (Position.Z > FloorZ || Momentum.Z > 0.0f)
	{
		// DOOM gravity: momz -= GRAVITY (FRACUNIT per tic)
		Momentum.Z -= DOOM_GRAVITY;
	}
}

// =============================================================================
// SpawnMobj - Factory method, port of P_SpawnMobj
// =============================================================================

ADoomEntity* ADoomEntity::SpawnMobj(UObject* WorldContextObject, FVector Location, EDoomMobjType MobjType)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Ensure entity info table is initialized
	UDoomEntityInfo::InitMobjInfoTable();

	const FDoomMobjInfo& Info = UDoomEntityInfo::GetMobjInfo(MobjType);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADoomEntity* Mobj = World->SpawnActor<ADoomEntity>(ADoomEntity::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
	if (!Mobj)
	{
		return nullptr;
	}

	// Initialize from mobjinfo table
	Mobj->Type = MobjType;
	Mobj->Position = Location;
	Mobj->Health = Info.SpawnHealth;
	Mobj->MobjFlags = Info.Flags;
	Mobj->Radius = Info.Radius * UDoomEntityInfo::DOOM_TO_UNREAL_SCALE;
	Mobj->Height = Info.Height * UDoomEntityInfo::DOOM_TO_UNREAL_SCALE;
	Mobj->Mass = Info.Mass;
	Mobj->Damage = Info.Damage;
	Mobj->Speed = Info.Speed;
	Mobj->ReactionTime = Info.ReactionTime;
	Mobj->CurrentStateIndex = Info.SpawnState;

	// Update collision sphere to match DOOM radius
	if (Mobj->CollisionComponent)
	{
		Mobj->CollisionComponent->SetSphereRadius(Mobj->Radius);
	}

	// Set initial floor/ceiling from spawn location
	// In full implementation, this queries the BSP/sector data
	Mobj->FloorZ = 0.0f;
	Mobj->CeilingZ = 10000.0f;

	// If spawned at ceiling, flip position
	if (Info.Flags & MF_SPAWNCEILING)
	{
		Mobj->Position.Z = Mobj->CeilingZ - Mobj->Height;
	}

	Mobj->SetActorLocation(Mobj->Position);

	return Mobj;
}

// =============================================================================
// RemoveMobj - port of P_RemoveMobj
// =============================================================================

void ADoomEntity::RemoveMobj()
{
	// Unlink from sector thing list
	if (SectorPrev)
	{
		SectorPrev->SectorNext = SectorNext;
	}
	if (SectorNext)
	{
		SectorNext->SectorPrev = SectorPrev;
	}

	// Unlink from blockmap
	if (BlockPrev)
	{
		BlockPrev->BlockNext = BlockNext;
	}
	if (BlockNext)
	{
		BlockNext->BlockPrev = BlockPrev;
	}

	// Clear references
	Target = nullptr;
	Tracer = nullptr;
	SectorNext = nullptr;
	SectorPrev = nullptr;
	BlockNext = nullptr;
	BlockPrev = nullptr;

	// Destroy the UE actor
	Destroy();
}

// =============================================================================
// SetState - port of P_SetMobjState
// =============================================================================

bool ADoomEntity::SetState(int32 StateIndex)
{
	// S_NULL (state 0) means remove the mobj
	if (StateIndex == 0)
	{
		CurrentStateIndex = 0;
		RemoveMobj();
		return false;
	}

	CurrentStateIndex = StateIndex;

	// In the full implementation, this would look up the state from a global
	// states table and set Sprite, Frame, Tics, and ActionFunction accordingly.
	// For now, broadcast the state change event.
	OnStateChanged.Broadcast(StateIndex);

	return true;
}
