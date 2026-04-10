// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_map.c - Movement, collision, and line attacks.

#include "DoomCollision.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "AI/DoomAI.h"

// =============================================================================
// P_CheckPosition - Query if a position is valid
// =============================================================================

bool UDoomCollision::CheckPosition(AActor* Thing, fixed_t X, fixed_t Y, FDoomMoveResult& OutResult)
{
	OutResult = FDoomMoveResult();

	if (!Thing)
		return false;

	UWorld* World = Thing->GetWorld();
	if (!World)
		return false;

	// Convert DOOM fixed-point target to UE5 world units
	const FVector CurLoc = Thing->GetActorLocation();
	const FVector TargetLoc(
		FixedToUE(X) * 100.0f,
		FixedToUE(Y) * 100.0f,
		CurLoc.Z
	);

	// Get the thing's radius - default to 20 DOOM units (~20cm in UE5)
	fixed_t ThingRadius = 20 * FRACUNIT;
	if (UDoomAIComponent* AI = Thing->FindComponentByClass<UDoomAIComponent>())
	{
		ThingRadius = AI->Radius;
	}

	const float Radius = FixedToUE(ThingRadius) * 100.0f;

	// Sweep a sphere from current position to target
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Thing);
	Params.bTraceComplex = false;

	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(
		Hit,
		CurLoc,
		TargetLoc,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params
	);

	if (bBlocked)
	{
		// Check if it's just a small step we can clamber over
		const float StepHeight = FixedToUE(MAX_STEP) * 100.0f;
		if (Hit.ImpactNormal.Z > 0.7f && Hit.ImpactPoint.Z - CurLoc.Z < StepHeight)
		{
			// Walkable step - allow
			OutResult.bSuccess = true;
			OutResult.FloorZ = UEToFixed(Hit.ImpactPoint.Z / 100.0f);
			OutResult.CeilingZ = UEToFixed((CurLoc.Z + 1000.0f) / 100.0f);
			OutResult.DropOffZ = OutResult.FloorZ;
			return true;
		}

		OutResult.bSuccess = false;
		return false;
	}

	// No obstruction - do a downward trace to find the floor
	const FVector FloorTraceStart = TargetLoc + FVector(0, 0, 100.0f);
	const FVector FloorTraceEnd = TargetLoc - FVector(0, 0, 10000.0f);

	FHitResult FloorHit;
	const bool bHasFloor = World->LineTraceSingleByChannel(
		FloorHit,
		FloorTraceStart,
		FloorTraceEnd,
		ECC_WorldStatic,
		Params
	);

	if (bHasFloor)
	{
		OutResult.FloorZ = UEToFixed(FloorHit.ImpactPoint.Z / 100.0f);
	}
	else
	{
		OutResult.FloorZ = UEToFixed(CurLoc.Z / 100.0f);
	}

	// Ceiling trace
	const FVector CeilTraceEnd = TargetLoc + FVector(0, 0, 10000.0f);
	FHitResult CeilHit;
	if (World->LineTraceSingleByChannel(CeilHit, TargetLoc, CeilTraceEnd, ECC_WorldStatic, Params))
	{
		OutResult.CeilingZ = UEToFixed(CeilHit.ImpactPoint.Z / 100.0f);
	}
	else
	{
		OutResult.CeilingZ = OutResult.FloorZ + 128 * FRACUNIT;
	}

	OutResult.DropOffZ = OutResult.FloorZ;
	OutResult.bSuccess = true;
	OutResult.bFloatOk = true;
	return true;
}

// =============================================================================
// P_TryMove - Attempt to move, returns false if blocked
// =============================================================================

bool UDoomCollision::TryMove(AActor* Thing, fixed_t X, fixed_t Y)
{
	if (!Thing)
		return false;

	FDoomMoveResult Result;
	if (!CheckPosition(Thing, X, Y, Result))
		return false;

	// Validate step height and ceiling clearance
	const fixed_t CurFloorZ = UEToFixed(Thing->GetActorLocation().Z / 100.0f);

	// Get thing height
	fixed_t ThingHeight = 56 * FRACUNIT;
	if (UDoomAIComponent* AI = Thing->FindComponentByClass<UDoomAIComponent>())
	{
		ThingHeight = AI->Height;
	}

	// Ceiling clearance check
	if (Result.CeilingZ - Result.FloorZ < ThingHeight)
		return false;

	// Step-up limit check (24 units max)
	if (Result.FloorZ - CurFloorZ > MAX_STEP)
		return false;

	// Drop-off check (monsters can't drop off cliffs unless MF_DROPOFF)
	int32 ThingFlags = 0;
	if (UDoomAIComponent* AI = Thing->FindComponentByClass<UDoomAIComponent>())
	{
		ThingFlags = AI->Flags;
	}

	if (!(ThingFlags & (MF_DROPOFF | MF_FLOAT)))
	{
		if (Result.FloorZ - Result.DropOffZ > MAX_STEP)
			return false;
	}

	// All checks passed - actually move
	const FVector NewLoc(
		FixedToUE(X) * 100.0f,
		FixedToUE(Y) * 100.0f,
		FixedToUE(Result.FloorZ) * 100.0f
	);

	Thing->SetActorLocation(NewLoc, false);
	return true;
}

// =============================================================================
// P_TeleportMove - Force move, telefragging blockers
// =============================================================================

bool UDoomCollision::TeleportMove(AActor* Thing, fixed_t X, fixed_t Y)
{
	if (!Thing)
		return false;

	UWorld* World = Thing->GetWorld();
	if (!World)
		return false;

	const FVector TargetLoc(
		FixedToUE(X) * 100.0f,
		FixedToUE(Y) * 100.0f,
		Thing->GetActorLocation().Z
	);

	// Get thing's radius
	fixed_t ThingRadius = 20 * FRACUNIT;
	if (UDoomAIComponent* AI = Thing->FindComponentByClass<UDoomAIComponent>())
	{
		ThingRadius = AI->Radius;
	}

	// Find overlapping actors in a sphere
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Thing);

	World->OverlapMultiByChannel(
		Overlaps,
		TargetLoc,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(FixedToUE(ThingRadius) * 100.0f),
		Params
	);

	// Telefrag any shootable things in the way (10000 damage)
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* Other = Overlap.GetActor())
		{
			if (UDoomAIComponent* AI = Other->FindComponentByClass<UDoomAIComponent>())
			{
				if (AI->Flags & MF_SHOOTABLE)
				{
					// Damage for 10000 (instant death)
					AI->Health -= 10000;
				}
			}
		}
	}

	// Force the move
	Thing->SetActorLocation(TargetLoc, false);
	return true;
}

// =============================================================================
// P_AimLineAttack - Auto-aim target search
// =============================================================================

int32 UDoomCollision::AimLineAttack(AActor* Shooter, angle_t Angle, fixed_t Distance,
	AActor*& OutLineTarget)
{
	OutLineTarget = nullptr;

	if (!Shooter)
		return 0;

	UWorld* World = Shooter->GetWorld();
	if (!World)
		return 0;

	const FVector Start = Shooter->GetActorLocation() + FVector(0, 0, 50.0f); // eye height
	const float DistUE = FixedToUE(Distance) * 100.0f;

	// Convert DOOM BAM angle to UE5 direction
	const float AngleDeg = static_cast<float>(Angle) * (360.0f / 4294967296.0f);
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	const FVector Dir(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.0f);
	const FVector End = Start + Dir * DistUE;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Shooter);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (UDoomAIComponent* AI = HitActor->FindComponentByClass<UDoomAIComponent>())
			{
				if (AI->Flags & MF_SHOOTABLE)
				{
					OutLineTarget = HitActor;
					// Compute slope (vertical aim) to target center
					const float DeltaZ = HitActor->GetActorLocation().Z - Start.Z;
					const float DistHorz = FVector::Dist2D(HitActor->GetActorLocation(), Start);
					if (DistHorz > 0.01f)
					{
						return UEToFixed(DeltaZ / DistHorz);
					}
				}
			}
		}
	}

	return 0;
}

// =============================================================================
// P_LineAttack - Hitscan with damage
// =============================================================================

FDoomLineAttackResult UDoomCollision::LineAttack(AActor* Shooter, angle_t Angle,
	fixed_t Distance, int32 Slope, int32 Damage)
{
	FDoomLineAttackResult Result;

	if (!Shooter)
		return Result;

	UWorld* World = Shooter->GetWorld();
	if (!World)
		return Result;

	const FVector Start = Shooter->GetActorLocation() + FVector(0, 0, 50.0f);
	const float DistUE = FixedToUE(Distance) * 100.0f;

	const float AngleDeg = static_cast<float>(Angle) * (360.0f / 4294967296.0f);
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);

	// Apply slope to Z component
	const float SlopeF = static_cast<float>(Slope) / FRACUNIT;
	FVector Dir(FMath::Cos(AngleRad), FMath::Sin(AngleRad), SlopeF);
	Dir.Normalize();

	const FVector End = Start + Dir * DistUE;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Shooter);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
	{
		Result.bHit = true;
		Result.HitActor = Hit.GetActor();
		Result.HitLocation = Hit.ImpactPoint;
		Result.HitNormal = Hit.ImpactNormal;
		Result.Slope = Slope;

		// Damage the target
		if (Result.HitActor)
		{
			if (UDoomAIComponent* AI = Result.HitActor->FindComponentByClass<UDoomAIComponent>())
			{
				if (AI->Flags & MF_SHOOTABLE)
				{
					AI->Health -= Damage;
					// Flag as just hit so monster fights back
					AI->Flags |= MF_JUSTHIT;
				}
			}
		}
	}

	return Result;
}

// =============================================================================
// P_RadiusAttack - Splash damage
// =============================================================================

void UDoomCollision::RadiusAttack(AActor* Spot, AActor* Source, int32 Damage)
{
	if (!Spot)
		return;

	UWorld* World = Spot->GetWorld();
	if (!World)
		return;

	// Radius = Damage (faithful to original: 128 damage rocket = 128 unit radius)
	const float RadiusUE = FixedToUE(Damage * FRACUNIT) * 100.0f;
	const FVector Center = Spot->GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Spot);
	if (Source)
		Params.AddIgnoredActor(Source);

	World->OverlapMultiByChannel(
		Overlaps,
		Center,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(RadiusUE),
		Params
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Victim = Overlap.GetActor();
		if (!Victim)
			continue;

		UDoomAIComponent* AI = Victim->FindComponentByClass<UDoomAIComponent>();
		if (!AI || !(AI->Flags & MF_SHOOTABLE))
			continue;

		// Distance-based falloff
		const float Dist = FVector::Dist(Victim->GetActorLocation(), Center);
		const float DistDoom = Dist / 100.0f; // UE5 -> DOOM units

		// Original formula: dist = P_AproxDistance - radius, damage *= (Damage - dist)
		// Simplified: linear falloff
		int32 FalloffDamage = static_cast<int32>(Damage * (1.0f - Dist / RadiusUE));
		if (FalloffDamage < 0)
			FalloffDamage = 0;

		AI->Health -= FalloffDamage;
	}
}

// =============================================================================
// P_ThingHeightClip - Resync Z after sector move
// =============================================================================

bool UDoomCollision::ThingHeightClip(AActor* Thing)
{
	if (!Thing)
		return false;

	UWorld* World = Thing->GetWorld();
	if (!World)
		return false;

	// Trace down to find floor
	const FVector Start = Thing->GetActorLocation() + FVector(0, 0, 100.0f);
	const FVector End = Start - FVector(0, 0, 10000.0f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Thing);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		FVector Loc = Thing->GetActorLocation();
		Loc.Z = Hit.ImpactPoint.Z;
		Thing->SetActorLocation(Loc, false);
		return true;
	}

	return false;
}

// =============================================================================
// P_SlideMove - Slide along walls
// =============================================================================

void UDoomCollision::SlideMove(AActor* Mo)
{
	if (!Mo)
		return;

	// Simplified slide: if TryMove fails, project velocity onto wall tangent
	// and try again. UE5's CharacterMovement handles this natively for pawns,
	// so this is a stub for monsters/things that don't use CharacterMovement.
	UWorld* World = Mo->GetWorld();
	if (!World)
		return;

	// Get current velocity (this assumes it's stored on the actor somehow)
	// For a full implementation we'd need to track momx/momy per thing.
	// UE5 integration: delegate to CharacterMovementComponent if present.
}
