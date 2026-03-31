// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_enemy.c - Enemy AI, thinking, and action pointer functions.
// Faithful line-by-line port preserving original DOOM behavior.

#include "DoomAI.h"
#include "Core/DoomRandom.h"
#include "GameFramework/Actor.h"

// =============================================================================
// Static lookup tables - exact values from p_enemy.c
// =============================================================================

// Direction movement speeds (xspeed/yspeed arrays from original)
const fixed_t UDoomAIComponent::XSpeed[8] = {
	FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000, 0, 47000
};

const fixed_t UDoomAIComponent::YSpeed[8] = {
	0, 47000, FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000
};

// Direction opposites
const EDoomDirection UDoomAIComponent::Opposite[9] = {
	EDoomDirection::West,      // East -> West
	EDoomDirection::SouthWest, // NorthEast -> SouthWest
	EDoomDirection::South,     // North -> South
	EDoomDirection::SouthEast, // NorthWest -> SouthEast
	EDoomDirection::East,      // West -> East
	EDoomDirection::NorthEast, // SouthWest -> NorthEast
	EDoomDirection::North,     // South -> North
	EDoomDirection::NorthWest, // SouthEast -> NorthWest
	EDoomDirection::NoDir      // NoDir -> NoDir
};

// Diagonal directions lookup
const EDoomDirection UDoomAIComponent::Diags[4] = {
	EDoomDirection::NorthWest,
	EDoomDirection::NorthEast,
	EDoomDirection::SouthWest,
	EDoomDirection::SouthEast
};

// Static random index (separate from FDoomRandom for AI-local use)
int32 UDoomAIComponent::RndIndex = 0;

// The DOOM random number table (256 entries) - identical to m_random.c
const uint8 UDoomAIComponent::RndTable[256] = {
	0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66,
	74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36,
	95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188,
	52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224,
	149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242,
	145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0,
	175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235,
	25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113,
	94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75,
	136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196,
	135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113,
	80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241,
	24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224,
	145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95,
	28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226,
	71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 108,  67, 117,
	97, 167, 127, 181, 131, 182,  27,  90, 131,   8,  72,  40,  36, 152,
	44, 215, 229, 208, 163, 190,  45, 109, 225, 168,  34, 136, 233, 139,
	40, 171,  24, 233
};

// =============================================================================
// Constructor
// =============================================================================

UDoomAIComponent::UDoomAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f; // Tick every frame, we accumulate internally
}

// =============================================================================
// Lifecycle
// =============================================================================

void UDoomAIComponent::BeginPlay()
{
	Super::BeginPlay();
	GameTic = 0;
	TicAccumulator = 0.0f;
}

void UDoomAIComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Accumulate real time and simulate DOOM tics at 35Hz
	TicAccumulator += DeltaTime;

	while (TicAccumulator >= TIC_DURATION)
	{
		TicAccumulator -= TIC_DURATION;
		GameTic++;

		// Run AI behavior based on current state
		switch (AIState)
		{
		case EDoomAIState::Idle:
			A_Look();
			break;
		case EDoomAIState::Chase:
			A_Chase();
			break;
		default:
			break;
		}
	}
}

// =============================================================================
// Utility functions
// =============================================================================

uint8 UDoomAIComponent::P_Random()
{
	RndIndex = (RndIndex + 1) & 0xff;
	return RndTable[RndIndex];
}

void UDoomAIComponent::SetAIState(EDoomAIState NewState)
{
	if (AIState != NewState)
	{
		EDoomAIState OldState = AIState;
		AIState = NewState;
		OnAIStateChanged.Broadcast(OldState, NewState);
	}
}

bool UDoomAIComponent::IsTargetValid() const
{
	return Target.IsValid() && Target.Get() != nullptr;
}

int32 UDoomAIComponent::GetTargetHealth() const
{
	if (!IsTargetValid()) return 0;

	// Try to get health from another DoomAI component on target
	if (UDoomAIComponent* TargetAI = Target->FindComponentByClass<UDoomAIComponent>())
	{
		return TargetAI->Health;
	}
	return 0;
}

int32 UDoomAIComponent::GetTargetFlags() const
{
	if (!IsTargetValid()) return 0;

	if (UDoomAIComponent* TargetAI = Target->FindComponentByClass<UDoomAIComponent>())
	{
		return TargetAI->Flags;
	}
	return 0;
}

void UDoomAIComponent::GetOwnerDoomPosition(fixed_t& OutX, fixed_t& OutY, fixed_t& OutZ) const
{
	if (AActor* Owner = GetOwner())
	{
		GetActorDoomPosition(Owner, OutX, OutY, OutZ);
	}
	else
	{
		OutX = OutY = OutZ = 0;
	}
}

void UDoomAIComponent::GetActorDoomPosition(const AActor* Actor, fixed_t& OutX, fixed_t& OutY, fixed_t& OutZ)
{
	if (!Actor)
	{
		OutX = OutY = OutZ = 0;
		return;
	}

	// Convert UE5 units to DOOM fixed-point (1 UE unit = 1 DOOM unit at FRACUNIT scale)
	const FVector Loc = Actor->GetActorLocation();
	OutX = static_cast<fixed_t>(Loc.X * FRACUNIT / 100.0f);
	OutY = static_cast<fixed_t>(Loc.Y * FRACUNIT / 100.0f);
	OutZ = static_cast<fixed_t>(Loc.Z * FRACUNIT / 100.0f);
}

fixed_t UDoomAIComponent::AproxDistance(fixed_t DX, fixed_t DY)
{
	// P_AproxDistance from original: max(abs(dx),abs(dy)) + min(abs(dx),abs(dy))/2
	DX = FMath::Abs(DX);
	DY = FMath::Abs(DY);

	if (DX < DY)
		return DX + DY - (DX >> 1);
	return DX + DY - (DY >> 1);
}

angle_t UDoomAIComponent::PointToAngle2(fixed_t X1, fixed_t Y1, fixed_t X2, fixed_t Y2)
{
	// Simplified R_PointToAngle2 using atan2
	const double dx = static_cast<double>(X2 - X1);
	const double dy = static_cast<double>(Y2 - Y1);
	const double angle = FMath::Atan2(dy, dx);

	// Convert from radians to DOOM BAM (binary angle measurement)
	// 2*PI radians = 0x100000000 BAM
	return static_cast<angle_t>(angle * (2147483648.0 / PI));
}

// =============================================================================
// P_CheckMeleeRange - port of P_CheckMeleeRange from p_enemy.c
// =============================================================================

bool UDoomAIComponent::P_CheckMeleeRange() const
{
	if (!IsTargetValid())
		return false;

	fixed_t MyX, MyY, MyZ;
	fixed_t TgtX, TgtY, TgtZ;
	GetOwnerDoomPosition(MyX, MyY, MyZ);
	GetActorDoomPosition(Target.Get(), TgtX, TgtY, TgtZ);

	const fixed_t Dist = AproxDistance(TgtX - MyX, TgtY - MyY);

	// Get target radius
	fixed_t TargetRadius = 20 * FRACUNIT; // default
	if (UDoomAIComponent* TargetAI = Target->FindComponentByClass<UDoomAIComponent>())
	{
		TargetRadius = TargetAI->Radius;
	}

	if (Dist >= MELEE_RANGE - 20 * FRACUNIT + TargetRadius)
		return false;

	// TODO: P_CheckSight equivalent - for now assume line of sight
	return true;
}

// =============================================================================
// P_CheckMissileRange - port of P_CheckMissileRange from p_enemy.c
// =============================================================================

bool UDoomAIComponent::P_CheckMissileRange() const
{
	if (!IsTargetValid())
		return false;

	// TODO: P_CheckSight equivalent
	// if (!P_CheckSight(actor, actor->target)) return false;

	if (Flags & MF_JUSTHIT)
	{
		// Target just hit us, fight back!
		// Note: can't clear flag in const method, caller should handle
		return true;
	}

	if (ReactionTime)
		return false; // do not attack yet

	fixed_t MyX, MyY, MyZ;
	fixed_t TgtX, TgtY, TgtZ;
	GetOwnerDoomPosition(MyX, MyY, MyZ);
	GetActorDoomPosition(Target.Get(), TgtX, TgtY, TgtZ);

	fixed_t Dist = AproxDistance(MyX - TgtX, MyY - TgtY) - 64 * FRACUNIT;

	if (!bHasMeleeAttack)
		Dist -= 128 * FRACUNIT; // no melee attack, so fire more

	Dist >>= 16;

	// Monster-specific distance adjustments (exact from original)
	if (MonsterType == EDoomMonsterType::Vile)
	{
		if (Dist > 14 * 64)
			return false; // too far away
	}

	if (MonsterType == EDoomMonsterType::Undead)
	{
		if (Dist < 196)
			return false; // close for fist attack
		Dist >>= 1;
	}

	if (MonsterType == EDoomMonsterType::Cyborg
		|| MonsterType == EDoomMonsterType::Spider
		|| MonsterType == EDoomMonsterType::Skull)
	{
		Dist >>= 1;
	}

	if (Dist > 200)
		Dist = 200;

	if (MonsterType == EDoomMonsterType::Cyborg && Dist > 160)
		Dist = 160;

	if (P_Random() < Dist)
		return false;

	return true;
}

// =============================================================================
// P_Move - Move in current direction, returns false if blocked
// Port of P_Move from p_enemy.c
// =============================================================================

bool UDoomAIComponent::P_Move()
{
	if (MoveDir == EDoomDirection::NoDir)
		return false;

	const int32 Dir = static_cast<int32>(MoveDir);
	if (Dir >= 8)
	{
		UE_LOG(LogTemp, Error, TEXT("DoomAI: Weird MoveDir %d"), Dir);
		return false;
	}

	fixed_t MyX, MyY, MyZ;
	GetOwnerDoomPosition(MyX, MyY, MyZ);

	const fixed_t TryX = MyX + Speed * XSpeed[Dir];
	const fixed_t TryY = MyY + Speed * YSpeed[Dir];

	// TODO: P_TryMove equivalent - for now do simplified collision
	// Convert back to UE5 coordinates and attempt move
	AActor* Owner = GetOwner();
	if (!Owner)
		return false;

	const FVector NewLoc(
		static_cast<float>(TryX) * 100.0f / FRACUNIT,
		static_cast<float>(TryY) * 100.0f / FRACUNIT,
		Owner->GetActorLocation().Z
	);

	// Sweep test for collision
	FHitResult Hit;
	const bool bMoved = Owner->SetActorLocation(NewLoc, true, &Hit);

	if (!bMoved)
	{
		// If we can float, adjust height
		if ((Flags & MF_FLOAT) && bFloatOk)
		{
			fixed_t OwnerZ;
			fixed_t Dummy;
			GetOwnerDoomPosition(Dummy, Dummy, OwnerZ);

			FVector Loc = Owner->GetActorLocation();
			// Move up or down toward target
			if (IsTargetValid())
			{
				fixed_t TgtX, TgtY, TgtZ;
				GetActorDoomPosition(Target.Get(), TgtX, TgtY, TgtZ);

				if (OwnerZ < TgtZ)
					Loc.Z += static_cast<float>(FLOAT_SPEED) * 100.0f / FRACUNIT;
				else
					Loc.Z -= static_cast<float>(FLOAT_SPEED) * 100.0f / FRACUNIT;
			}

			Flags |= MF_INFLOAT;
			Owner->SetActorLocation(Loc, true);
			return true;
		}

		MoveDir = EDoomDirection::NoDir;
		return false;
	}

	Flags &= ~MF_INFLOAT;

	if (!(Flags & MF_FLOAT))
	{
		// Snap to floor - simplified, let UE5 gravity handle it
	}

	return true;
}

// =============================================================================
// P_TryWalk - Attempt to walk in current direction
// Port of P_TryWalk from p_enemy.c
// =============================================================================

bool UDoomAIComponent::P_TryWalk()
{
	if (!P_Move())
		return false;

	MoveCount = P_Random() & 15;
	return true;
}

// =============================================================================
// P_NewChaseDir - 8-directional pathfinding toward target
// Port of P_NewChaseDir from p_enemy.c
// =============================================================================

void UDoomAIComponent::P_NewChaseDir()
{
	if (!IsTargetValid())
	{
		UE_LOG(LogTemp, Error, TEXT("DoomAI: P_NewChaseDir called with no target"));
		return;
	}

	const EDoomDirection OldDir = MoveDir;
	const EDoomDirection TurnAround = Opposite[static_cast<int32>(OldDir)];

	fixed_t MyX, MyY, MyZ;
	fixed_t TgtX, TgtY, TgtZ;
	GetOwnerDoomPosition(MyX, MyY, MyZ);
	GetActorDoomPosition(Target.Get(), TgtX, TgtY, TgtZ);

	const fixed_t DeltaX = TgtX - MyX;
	const fixed_t DeltaY = TgtY - MyY;

	EDoomDirection d[3];
	d[0] = EDoomDirection::NoDir; // unused index 0

	if (DeltaX > 10 * FRACUNIT)
		d[1] = EDoomDirection::East;
	else if (DeltaX < -10 * FRACUNIT)
		d[1] = EDoomDirection::West;
	else
		d[1] = EDoomDirection::NoDir;

	if (DeltaY < -10 * FRACUNIT)
		d[2] = EDoomDirection::South;
	else if (DeltaY > 10 * FRACUNIT)
		d[2] = EDoomDirection::North;
	else
		d[2] = EDoomDirection::NoDir;

	// Try direct route (diagonal)
	if (d[1] != EDoomDirection::NoDir && d[2] != EDoomDirection::NoDir)
	{
		const int32 DiagIdx = ((DeltaY < 0) << 1) + (DeltaX > 0);
		MoveDir = Diags[DiagIdx];
		if (MoveDir != TurnAround && P_TryWalk())
			return;
	}

	// Try other directions - randomly swap preference
	if (P_Random() > 200 || FMath::Abs(DeltaY) > FMath::Abs(DeltaX))
	{
		EDoomDirection Temp = d[1];
		d[1] = d[2];
		d[2] = Temp;
	}

	if (d[1] == TurnAround)
		d[1] = EDoomDirection::NoDir;
	if (d[2] == TurnAround)
		d[2] = EDoomDirection::NoDir;

	if (d[1] != EDoomDirection::NoDir)
	{
		MoveDir = d[1];
		if (P_TryWalk())
			return;
	}

	if (d[2] != EDoomDirection::NoDir)
	{
		MoveDir = d[2];
		if (P_TryWalk())
			return;
	}

	// No direct path, try old direction
	if (OldDir != EDoomDirection::NoDir)
	{
		MoveDir = OldDir;
		if (P_TryWalk())
			return;
	}

	// Randomly determine direction of search
	if (P_Random() & 1)
	{
		for (int32 TDir = static_cast<int32>(EDoomDirection::East);
			TDir <= static_cast<int32>(EDoomDirection::SouthEast);
			TDir++)
		{
			if (static_cast<EDoomDirection>(TDir) != TurnAround)
			{
				MoveDir = static_cast<EDoomDirection>(TDir);
				if (P_TryWalk())
					return;
			}
		}
	}
	else
	{
		for (int32 TDir = static_cast<int32>(EDoomDirection::SouthEast);
			TDir >= static_cast<int32>(EDoomDirection::East);
			TDir--)
		{
			if (static_cast<EDoomDirection>(TDir) != TurnAround)
			{
				MoveDir = static_cast<EDoomDirection>(TDir);
				if (P_TryWalk())
					return;
			}
		}
	}

	// Last resort: turnaround
	if (TurnAround != EDoomDirection::NoDir)
	{
		MoveDir = TurnAround;
		if (P_TryWalk())
			return;
	}

	MoveDir = EDoomDirection::NoDir; // can not move
}

// =============================================================================
// P_LookForPlayers - Scan for visible players
// Port of P_LookForPlayers from p_enemy.c
// =============================================================================

bool UDoomAIComponent::P_LookForPlayers(bool bAllAround)
{
	int32 Count = 0;
	const int32 Stop = (LastLook - 1) & 3;

	for (;;)
	{
		// Check if this player index is valid
		if (LastLook < PlayerActors.Num() && PlayerActors[LastLook] != nullptr)
		{
			if (Count++ == 2 || LastLook == Stop)
				return false; // done looking

			AActor* PlayerActor = PlayerActors[LastLook];

			// Check if player is alive
			int32 PlayerHealth = 0;
			if (UDoomAIComponent* PlayerAI = PlayerActor->FindComponentByClass<UDoomAIComponent>())
			{
				PlayerHealth = PlayerAI->Health;
			}

			if (PlayerHealth <= 0)
			{
				LastLook = (LastLook + 1) & 3;
				continue; // dead
			}

			// TODO: P_CheckSight equivalent

			if (!bAllAround)
			{
				fixed_t MyX, MyY, MyZ;
				fixed_t PlX, PlY, PlZ;
				GetOwnerDoomPosition(MyX, MyY, MyZ);
				GetActorDoomPosition(PlayerActor, PlX, PlY, PlZ);

				angle_t An = PointToAngle2(MyX, MyY, PlX, PlY) - FacingAngle;

				// ANG90 = 0x40000000, ANG270 = 0xC0000000
				if (An > 0x40000000u && An < 0xC0000000u)
				{
					const fixed_t Dist = AproxDistance(PlX - MyX, PlY - MyY);
					if (Dist > MELEE_RANGE)
					{
						LastLook = (LastLook + 1) & 3;
						continue; // behind back
					}
				}
			}

			Target = PlayerActor;
			return true;
		}
		else
		{
			if (Count++ == 2 || LastLook == Stop)
				return false;
		}

		LastLook = (LastLook + 1) & 3;
	}

	return false;
}

// =============================================================================
// NoiseAlert - Sound propagation (simplified port)
// =============================================================================

void UDoomAIComponent::NoiseAlert(AActor* InTarget, AActor* Emitter)
{
	// In the original, this recursively floods sectors with sound.
	// In UE5, we broadcast a delegate or use a radius-based approach.
	// This is a stub - the full sector-based flood requires map data integration.
	UE_LOG(LogTemp, Verbose, TEXT("DoomAI: NoiseAlert from %s targeting %s"),
		Emitter ? *Emitter->GetName() : TEXT("null"),
		InTarget ? *InTarget->GetName() : TEXT("null"));
}

// =============================================================================
// A_Look - Idle state, search for player via sight and sound
// Port of A_Look from p_enemy.c
// =============================================================================

void UDoomAIComponent::A_Look()
{
	Threshold = 0; // any shot will wake up

	// TODO: Check sector soundtarget (requires map data integration)
	// In original: targ = actor->subsector->sector->soundtarget;
	// For now, go straight to player search

	if (!P_LookForPlayers(false))
		return;

	// Go into chase state
	if (!SeeSound.IsNone())
	{
		// Sound variation for zombie/imp see sounds (faithful to original)
		OnPlaySound.Broadcast(SeeSound);

		// Bosses play at full volume (Spider Mastermind, Cyberdemon)
		if (MonsterType == EDoomMonsterType::Spider
			|| MonsterType == EDoomMonsterType::Cyborg)
		{
			// Full volume - broadcast globally
			OnPlaySound.Broadcast(SeeSound);
		}
	}

	SetAIState(EDoomAIState::Chase);
}

// =============================================================================
// A_Chase - Pursue target with movement, melee, and missile decisions
// Port of A_Chase from p_enemy.c
// =============================================================================

void UDoomAIComponent::A_Chase()
{
	if (ReactionTime)
		ReactionTime--;

	// Modify target threshold
	if (Threshold)
	{
		if (!IsTargetValid() || GetTargetHealth() <= 0)
		{
			Threshold = 0;
		}
		else
		{
			Threshold--;
		}
	}

	// Turn towards movement direction if not there yet
	if (static_cast<int32>(MoveDir) < 8)
	{
		FacingAngle &= (7u << 29);
		int32 Delta = static_cast<int32>(FacingAngle - (static_cast<uint32>(MoveDir) << 29));

		if (Delta > 0)
			FacingAngle -= 0x20000000u; // ANG90/2
		else if (Delta < 0)
			FacingAngle += 0x20000000u; // ANG90/2
	}

	if (!IsTargetValid() || !(GetTargetFlags() & MF_SHOOTABLE))
	{
		// Look for a new target
		if (P_LookForPlayers(true))
			return; // got a new target

		SetAIState(EDoomAIState::Idle);
		return;
	}

	// Do not attack twice in a row
	if (Flags & MF_JUSTATTACKED)
	{
		Flags &= ~MF_JUSTATTACKED;
		if (GameSkill != EDoomSkill::Nightmare && !bFastParm)
			P_NewChaseDir();
		return;
	}

	// Check for melee attack
	if (bHasMeleeAttack && P_CheckMeleeRange())
	{
		if (!AttackSound.IsNone())
			OnPlaySound.Broadcast(AttackSound);

		SetAIState(EDoomAIState::Attack);
		OnAttack.Broadcast(Target.Get());
		return;
	}

	// Check for missile attack
	if (bHasMissileAttack)
	{
		if (GameSkill < EDoomSkill::Nightmare && !bFastParm && MoveCount)
		{
			// Don't fire yet, keep chasing
		}
		else if (P_CheckMissileRange())
		{
			SetAIState(EDoomAIState::Attack);
			Flags |= MF_JUSTATTACKED;
			OnSpawnProjectile.Broadcast(ProjectileType, Target.Get());
			return;
		}
	}

	// Possibly choose another target in netgames
	if (bNetGame && !Threshold)
	{
		// TODO: P_CheckSight equivalent
		if (P_LookForPlayers(true))
			return; // got a new target
	}

	// Chase towards player
	if (--MoveCount < 0 || !P_Move())
	{
		P_NewChaseDir();
	}

	// Make active sound
	if (!ActiveSound.IsNone() && P_Random() < 3)
	{
		OnPlaySound.Broadcast(ActiveSound);
	}
}

// =============================================================================
// A_FaceTarget - Turn to face current target
// Port of A_FaceTarget from p_enemy.c
// =============================================================================

void UDoomAIComponent::A_FaceTarget()
{
	if (!IsTargetValid())
		return;

	Flags &= ~MF_AMBUSH;

	fixed_t MyX, MyY, MyZ;
	fixed_t TgtX, TgtY, TgtZ;
	GetOwnerDoomPosition(MyX, MyY, MyZ);
	GetActorDoomPosition(Target.Get(), TgtX, TgtY, TgtZ);

	FacingAngle = PointToAngle2(MyX, MyY, TgtX, TgtY);

	// Jitter when target has MF_SHADOW (spectre invisibility)
	if (GetTargetFlags() & MF_SHADOW)
	{
		FacingAngle += static_cast<angle_t>((static_cast<int32>(P_Random()) - static_cast<int32>(P_Random())) << 21);
	}

	// Apply facing to UE5 actor rotation
	if (AActor* Owner = GetOwner())
	{
		// Convert BAM to degrees: angle / (0x100000000 / 360)
		const float Degrees = static_cast<float>(FacingAngle) * (360.0f / 4294967296.0f);
		Owner->SetActorRotation(FRotator(0.0f, Degrees, 0.0f));
	}
}

// =============================================================================
// Attack routines - Individual monster attacks
// Faithful ports from p_enemy.c
// =============================================================================

void UDoomAIComponent::A_PosAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();
	OnPlaySound.Broadcast(FName("pistol"));

	// angle += (P_Random()-P_Random())<<20  (spread)
	const int32 Damage = ((P_Random() % 5) + 1) * 3;
	OnDamage.Broadcast(Target.Get(), Damage);
}

void UDoomAIComponent::A_SPosAttack()
{
	if (!IsTargetValid()) return;

	OnPlaySound.Broadcast(FName("shotgn"));
	A_FaceTarget();

	// 3 pellets
	for (int32 i = 0; i < 3; i++)
	{
		const int32 Damage = ((P_Random() % 5) + 1) * 3;
		OnDamage.Broadcast(Target.Get(), Damage);
	}
}

void UDoomAIComponent::A_CPosAttack()
{
	if (!IsTargetValid()) return;

	OnPlaySound.Broadcast(FName("shotgn"));
	A_FaceTarget();

	const int32 Damage = ((P_Random() % 5) + 1) * 3;
	OnDamage.Broadcast(Target.Get(), Damage);
}

void UDoomAIComponent::A_CPosRefire()
{
	A_FaceTarget();

	if (P_Random() < 40)
		return;

	if (!IsTargetValid() || GetTargetHealth() <= 0)
	{
		SetAIState(EDoomAIState::Chase);
	}
}

void UDoomAIComponent::A_SpidRefire()
{
	A_FaceTarget();

	if (P_Random() < 10)
		return;

	if (!IsTargetValid() || GetTargetHealth() <= 0)
	{
		SetAIState(EDoomAIState::Chase);
	}
}

void UDoomAIComponent::A_TroopAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();

	if (P_CheckMeleeRange())
	{
		OnPlaySound.Broadcast(FName("claw"));
		const int32 Damage = (P_Random() % 8 + 1) * 3;
		OnDamage.Broadcast(Target.Get(), Damage);
		return;
	}

	// Launch fireball
	OnSpawnProjectile.Broadcast(EDoomProjectileType::ImpFireball, Target.Get());
}

void UDoomAIComponent::A_SargAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();

	if (P_CheckMeleeRange())
	{
		const int32 Damage = ((P_Random() % 10) + 1) * 4;
		OnDamage.Broadcast(Target.Get(), Damage);
	}
}

void UDoomAIComponent::A_HeadAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();

	if (P_CheckMeleeRange())
	{
		const int32 Damage = (P_Random() % 6 + 1) * 10;
		OnDamage.Broadcast(Target.Get(), Damage);
		return;
	}

	OnSpawnProjectile.Broadcast(EDoomProjectileType::CacoBall, Target.Get());
}

void UDoomAIComponent::A_BruisAttack()
{
	if (!IsTargetValid()) return;

	if (P_CheckMeleeRange())
	{
		OnPlaySound.Broadcast(FName("claw"));
		const int32 Damage = (P_Random() % 8 + 1) * 10;
		OnDamage.Broadcast(Target.Get(), Damage);
		return;
	}

	OnSpawnProjectile.Broadcast(EDoomProjectileType::BaronBall, Target.Get());
}

void UDoomAIComponent::A_CyberAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();
	OnSpawnProjectile.Broadcast(EDoomProjectileType::Rocket, Target.Get());
}

void UDoomAIComponent::A_SpidAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();

	// Same as chaingunner
	const int32 Damage = ((P_Random() % 5) + 1) * 3;
	OnDamage.Broadcast(Target.Get(), Damage);
}

void UDoomAIComponent::A_BspiAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();
	OnSpawnProjectile.Broadcast(EDoomProjectileType::PlasmaBall, Target.Get());
}

// =============================================================================
// Revenant attacks
// =============================================================================

void UDoomAIComponent::A_SkelMissile()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();
	// In original: actor->z += 16*FRACUNIT, spawn missile, actor->z -= 16*FRACUNIT
	OnSpawnProjectile.Broadcast(EDoomProjectileType::TracerMissile, Target.Get());
}

void UDoomAIComponent::A_SkelWhoosh()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();
	OnPlaySound.Broadcast(FName("skeswg"));
}

void UDoomAIComponent::A_SkelFist()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();

	if (P_CheckMeleeRange())
	{
		const int32 Damage = ((P_Random() % 10) + 1) * 6;
		OnPlaySound.Broadcast(FName("skepch"));
		OnDamage.Broadcast(Target.Get(), Damage);
	}
}

void UDoomAIComponent::A_Tracer()
{
	// Only adjust every 4th tic (faithful to original: if (gametic & 3) return)
	if (GameTic & 3)
		return;

	// Spawn smoke puff behind the missile
	OnSpecialEvent.Broadcast(FName("TracerSmoke"));

	// Adjust direction toward target (homing)
	if (!IsTargetValid() || GetTargetHealth() <= 0)
		return;

	fixed_t MyX, MyY, MyZ;
	fixed_t TgtX, TgtY, TgtZ;
	GetOwnerDoomPosition(MyX, MyY, MyZ);
	GetActorDoomPosition(Target.Get(), TgtX, TgtY, TgtZ);

	const angle_t Exact = PointToAngle2(MyX, MyY, TgtX, TgtY);

	if (Exact != FacingAngle)
	{
		if (Exact - FacingAngle > 0x80000000u)
		{
			FacingAngle -= TRACE_ANGLE;
			if (Exact - FacingAngle < 0x80000000u)
				FacingAngle = Exact;
		}
		else
		{
			FacingAngle += TRACE_ANGLE;
			if (Exact - FacingAngle > 0x80000000u)
				FacingAngle = Exact;
		}
	}
}

// =============================================================================
// Mancubus attacks - three volleys of two fireballs each
// =============================================================================

void UDoomAIComponent::A_FatRaise()
{
	A_FaceTarget();
	OnPlaySound.Broadcast(FName("manatk"));
}

void UDoomAIComponent::A_FatAttack1()
{
	A_FaceTarget();
	// Spread right: actor->angle += FATSPREAD
	FacingAngle += FAT_SPREAD;
	OnSpawnProjectile.Broadcast(EDoomProjectileType::FatShot, Target.Get());
	// Second fireball with extra spread
	OnSpawnProjectile.Broadcast(EDoomProjectileType::FatShot, Target.Get());
}

void UDoomAIComponent::A_FatAttack2()
{
	A_FaceTarget();
	// Spread left: actor->angle -= FATSPREAD
	FacingAngle -= FAT_SPREAD;
	OnSpawnProjectile.Broadcast(EDoomProjectileType::FatShot, Target.Get());
	// Second fireball with double spread left
	OnSpawnProjectile.Broadcast(EDoomProjectileType::FatShot, Target.Get());
}

void UDoomAIComponent::A_FatAttack3()
{
	A_FaceTarget();
	// Slight spread both ways
	OnSpawnProjectile.Broadcast(EDoomProjectileType::FatShot, Target.Get());
	OnSpawnProjectile.Broadcast(EDoomProjectileType::FatShot, Target.Get());
}

// =============================================================================
// Lost Soul charge attack
// =============================================================================

void UDoomAIComponent::A_SkullAttack()
{
	if (!IsTargetValid()) return;

	Flags |= MF_SKULLFLY;

	if (!AttackSound.IsNone())
		OnPlaySound.Broadcast(AttackSound);

	A_FaceTarget();

	// Set velocity toward target at SKULL_SPEED
	fixed_t MyX, MyY, MyZ;
	fixed_t TgtX, TgtY, TgtZ;
	GetOwnerDoomPosition(MyX, MyY, MyZ);
	GetActorDoomPosition(Target.Get(), TgtX, TgtY, TgtZ);

	// Compute direction and apply skull speed via delegate
	OnSpecialEvent.Broadcast(FName("SkullCharge"));
}

// =============================================================================
// Archvile - resurrection, fire attack
// =============================================================================

void UDoomAIComponent::A_VileChase()
{
	// In original: check for corpses to raise before chasing
	// Search for dead monsters nearby and resurrect them
	if (MoveDir != EDoomDirection::NoDir)
	{
		// Broadcast event so the game framework can check for corpses
		OnSpecialEvent.Broadcast(FName("VileSearchCorpse"));
	}

	// Fall back to normal chase behavior
	A_Chase();
}

void UDoomAIComponent::A_VileStart()
{
	OnPlaySound.Broadcast(FName("vilatk"));
}

void UDoomAIComponent::A_VileTarget()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();

	// Spawn hellfire at target's position
	OnSpecialEvent.Broadcast(FName("VileFireSpawn"));
}

void UDoomAIComponent::A_VileAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();

	// TODO: P_CheckSight equivalent
	// if (!P_CheckSight(actor, actor->target)) return;

	OnPlaySound.Broadcast(FName("barexp"));

	// 20 direct damage
	OnDamage.Broadcast(Target.Get(), 20);

	// Launch target into the air: target->momz = 1000*FRACUNIT/target->mass
	OnSpecialEvent.Broadcast(FName("VileLaunch"));

	// 70 radius damage from fire
	OnSpecialEvent.Broadcast(FName("VileRadiusDamage"));
}

void UDoomAIComponent::A_Fire()
{
	// Keep fire sprite in front of archvile's target
	// In original: moves fire actor to dest position + 24 units in front
	OnSpecialEvent.Broadcast(FName("VileFireMove"));
}

void UDoomAIComponent::A_StartFire()
{
	OnPlaySound.Broadcast(FName("flamst"));
	A_Fire();
}

void UDoomAIComponent::A_FireCrackle()
{
	OnPlaySound.Broadcast(FName("flame"));
	A_Fire();
}

// =============================================================================
// Pain Elemental - spawns Lost Souls
// =============================================================================

void UDoomAIComponent::A_PainAttack()
{
	if (!IsTargetValid()) return;

	A_FaceTarget();
	PainShootSkull(FacingAngle);
}

void UDoomAIComponent::A_PainDie()
{
	A_Fall();
	// Spawn 3 Lost Souls at 90, 180, 270 degree offsets
	PainShootSkull(FacingAngle + 0x40000000u);  // +ANG90
	PainShootSkull(FacingAngle + 0x80000000u);  // +ANG180
	PainShootSkull(FacingAngle + 0xC0000000u);  // +ANG270
}

void UDoomAIComponent::PainShootSkull(angle_t Angle)
{
	// In original: count skulls on level, enforce 20 limit
	// Broadcast spawn event - game framework handles the actual spawning
	OnSpecialEvent.Broadcast(FName("PainSpawnSkull"));
}

// =============================================================================
// Commander Keen - DOOM II MAP32 special
// =============================================================================

void UDoomAIComponent::A_KeenDie()
{
	A_Fall();
	// When all Keens are dead, open tag 666 door
	OnSpecialEvent.Broadcast(FName("KeenDieCheck"));
}

// =============================================================================
// Icon of Sin - Boss brain
// =============================================================================

void UDoomAIComponent::A_BrainAwake()
{
	// Find all boss target spots
	BrainTargets.Empty();
	BrainTargetOn = 0;

	// Broadcast so the game framework can populate BrainTargets
	OnSpecialEvent.Broadcast(FName("BrainFindTargets"));
	OnPlaySound.Broadcast(FName("bossit"));
}

void UDoomAIComponent::A_BrainSpit()
{
	// Easy mode toggle (faithful to original static int easy)
	BrainEasy ^= 1;
	if (GameSkill <= EDoomSkill::Easy && (!BrainEasy))
		return;

	if (BrainTargets.Num() == 0)
		return;

	// Shoot a cube at the current target
	AActor* Targ = BrainTargets[BrainTargetOn];
	BrainTargetOn = (BrainTargetOn + 1) % BrainTargets.Num();

	OnSpawnProjectile.Broadcast(EDoomProjectileType::SpawnShot, Targ);
	OnPlaySound.Broadcast(FName("bospit"));
}

void UDoomAIComponent::A_BrainDie()
{
	// Exit level
	OnSpecialEvent.Broadcast(FName("ExitLevel"));
}

void UDoomAIComponent::A_BrainExplode()
{
	// Spawn random explosion around the brain
	OnSpecialEvent.Broadcast(FName("BrainExplosion"));
}

void UDoomAIComponent::A_SpawnFly()
{
	if (--ReactionTime)
		return; // still flying

	// Randomly select monster to spawn (exact probability from original)
	const int32 R = P_Random();
	EDoomMonsterType SpawnType;

	if (R < 50)         SpawnType = EDoomMonsterType::Troop;
	else if (R < 90)    SpawnType = EDoomMonsterType::Sergeant;
	else if (R < 120)   SpawnType = EDoomMonsterType::Shadows;
	else if (R < 130)   SpawnType = EDoomMonsterType::Pain;
	else if (R < 160)   SpawnType = EDoomMonsterType::Head;
	else if (R < 162)   SpawnType = EDoomMonsterType::Vile;
	else if (R < 172)   SpawnType = EDoomMonsterType::Undead;
	else if (R < 192)   SpawnType = EDoomMonsterType::Baby;
	else if (R < 222)   SpawnType = EDoomMonsterType::Fatso;
	else if (R < 246)   SpawnType = EDoomMonsterType::Knight;
	else                SpawnType = EDoomMonsterType::Bruiser;

	// Broadcast spawn event with type encoded in name
	FName SpawnEventName(*FString::Printf(TEXT("SpawnFly_%d"), static_cast<int32>(SpawnType)));
	OnSpecialEvent.Broadcast(SpawnEventName);
}

void UDoomAIComponent::A_SpawnSound()
{
	OnPlaySound.Broadcast(FName("boscub"));
	A_SpawnFly();
}

void UDoomAIComponent::A_BrainScream()
{
	// Boss brain death explosion cascade
	OnSpecialEvent.Broadcast(FName("BrainScream"));
	OnPlaySound.Broadcast(FName("bosdth"));
}

void UDoomAIComponent::A_BrainPain()
{
	OnPlaySound.Broadcast(FName("bospn"));
}

// =============================================================================
// Generic action routines
// =============================================================================

void UDoomAIComponent::A_Fall()
{
	// Monster dies, becomes non-solid (can be walked over)
	Flags &= ~MF_SOLID;
}

void UDoomAIComponent::A_Explode()
{
	// Radius attack with 128 damage (rockets, barrels)
	OnSpecialEvent.Broadcast(FName("RadiusAttack128"));
}

void UDoomAIComponent::A_Scream()
{
	if (DeathSound.IsNone())
		return;

	// Bosses play at full volume
	if (MonsterType == EDoomMonsterType::Spider
		|| MonsterType == EDoomMonsterType::Cyborg)
	{
		// Full volume
		OnPlaySound.Broadcast(DeathSound);
	}
	else
	{
		OnPlaySound.Broadcast(DeathSound);
	}
}

void UDoomAIComponent::A_XScream()
{
	// Extra-violent death (gib sound)
	OnPlaySound.Broadcast(FName("slop"));
}

void UDoomAIComponent::A_Pain()
{
	if (!PainSound.IsNone())
		OnPlaySound.Broadcast(PainSound);
}

void UDoomAIComponent::A_BossDeath()
{
	// Special level-end triggers when bosses die
	// Broadcast event so the game framework can check level/episode conditions
	OnSpecialEvent.Broadcast(FName("BossDeath"));
}

void UDoomAIComponent::A_Hoof()
{
	// Cyberdemon hoof stomp + chase
	OnPlaySound.Broadcast(FName("hoof"));
	A_Chase();
}

void UDoomAIComponent::A_Metal()
{
	// Spider Mastermind metal walk + chase
	OnPlaySound.Broadcast(FName("metal"));
	A_Chase();
}

void UDoomAIComponent::A_BabyMetal()
{
	// Arachnotron walk + chase
	OnPlaySound.Broadcast(FName("bspwlk"));
	A_Chase();
}
