// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_sight.c - Line of sight checking.
//
// The original DOOM uses BSP traversal + the REJECT table for fast
// sector-to-sector visibility culling. In UE5 we replace that with direct
// line traces against world geometry, which is simpler and takes advantage
// of UE5's spatial acceleration structures.

#include "DoomSight.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"
#include "AI/DoomAI.h"

// Static stat counters
int32 UDoomSightChecker::SightCountRejected = 0;
int32 UDoomSightChecker::SightCountChecked = 0;

// =============================================================================
// P_CheckSight - Line of sight between two actors
// =============================================================================

bool UDoomSightChecker::CheckSight(AActor* T1, AActor* T2)
{
	if (!T1 || !T2)
	{
		SightCountRejected++;
		return false;
	}

	UWorld* World = T1->GetWorld();
	if (!World)
	{
		SightCountRejected++;
		return false;
	}

	// DOOM's sight origin is at 75% of the actor's height
	// (original: sightzstart = t1->z + t1->height - (t1->height>>2))
	fixed_t T1Height = 56 * FRACUNIT; // default
	if (UDoomAIComponent* AI = T1->FindComponentByClass<UDoomAIComponent>())
	{
		T1Height = AI->Height;
	}

	const float T1HeightF = static_cast<float>(T1Height) / FRACUNIT * 100.0f;
	const float EyeOffset = T1HeightF * 0.75f;

	const FVector Start = T1->GetActorLocation() + FVector(0, 0, EyeOffset);

	// Target position: aim at center mass of T2
	fixed_t T2Height = 56 * FRACUNIT;
	if (UDoomAIComponent* AI = T2->FindComponentByClass<UDoomAIComponent>())
	{
		T2Height = AI->Height;
	}
	const float T2HeightF = static_cast<float>(T2Height) / FRACUNIT * 100.0f;

	const FVector End = T2->GetActorLocation() + FVector(0, 0, T2HeightF * 0.5f);

	SightCountChecked++;

	// Line trace against world geometry only - ignore pawns/things
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(T1);
	Params.AddIgnoredActor(T2);
	Params.bTraceComplex = false;

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_WorldStatic,
		Params
	);

	return !bBlocked;
}

// =============================================================================
// Statistics
// =============================================================================

void UDoomSightChecker::GetSightCounts(int32& OutRejected, int32& OutChecked)
{
	OutRejected = SightCountRejected;
	OutChecked = SightCountChecked;
}

void UDoomSightChecker::ResetSightCounts()
{
	SightCountRejected = 0;
	SightCountChecked = 0;
}

// =============================================================================
// Low-level helpers (used for divline intersection in original BSP sight)
// Kept for compatibility with code that wants to do manual sight checks.
// =============================================================================

int32 UDoomSightChecker::DivLineSide(fixed_t X, fixed_t Y, fixed_t LineX, fixed_t LineY,
	fixed_t LineDX, fixed_t LineDY)
{
	// Port of P_DivlineSide from p_sight.c
	fixed_t DX, DY;
	fixed_t Left, Right;

	if (!LineDX)
	{
		if (X == LineX)
			return 2; // on the line
		if (X <= LineX)
			return LineDY > 0 ? 1 : 0;
		return LineDY < 0 ? 1 : 0;
	}

	if (!LineDY)
	{
		if (Y == LineY)
			return 2;
		if (Y <= LineY)
			return LineDX < 0 ? 1 : 0;
		return LineDX > 0 ? 1 : 0;
	}

	DX = (X - LineX);
	DY = (Y - LineY);

	Left = (LineDY >> FRACBITS) * (DX >> FRACBITS);
	Right = (DY >> FRACBITS) * (LineDX >> FRACBITS);

	if (Right < Left)
		return 0; // front side
	if (Left == Right)
		return 2;
	return 1; // back side
}

fixed_t UDoomSightChecker::InterceptVector2(fixed_t StraceX, fixed_t StraceY,
	fixed_t StraceDX, fixed_t StraceDY,
	fixed_t DivlX, fixed_t DivlY,
	fixed_t DivlDX, fixed_t DivlDY)
{
	// Port of P_InterceptVector2 from p_sight.c
	// Returns the fraction along the strace where it intersects divl
	fixed_t Frac;
	fixed_t Num;
	fixed_t Den;

	// Compute determinant
	Den = FMath::MultiplyAndDivide(DivlDY, StraceDX, FRACUNIT)
		- FMath::MultiplyAndDivide(DivlDX, StraceDY, FRACUNIT);

	if (Den == 0)
		return 0;

	Num = FMath::MultiplyAndDivide(DivlX - StraceX, DivlDY, FRACUNIT)
		+ FMath::MultiplyAndDivide(StraceY - DivlY, DivlDX, FRACUNIT);

	Frac = FMath::MultiplyAndDivide(Num, FRACUNIT, Den);

	return Frac;
}
