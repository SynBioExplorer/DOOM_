#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_sight.c - Line of sight checking.

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/DoomTypes.h"
#include "DoomSight.generated.h"

class AActor;

// =============================================================================
// UDoomSightChecker - Line of sight queries for AI
// Port of P_CheckSight from p_sight.c
// =============================================================================

UCLASS(BlueprintType)
class UNREALDOOM_API UDoomSightChecker : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * P_CheckSight: Can T1 see T2 through the level geometry?
	 * @param T1 Observer (eye position derived from height)
	 * @param T2 Target
	 * @return true if the sight line is unobstructed
	 *
	 * Uses DOOM's rule: eye height is (z + height - height/4) = 75% of actor height.
	 * Checks both horizontal line-of-sight and vertical slope clearance.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Sight")
	static bool CheckSight(AActor* T1, AActor* T2);

	/**
	 * Statistics for performance tracking (port of sightcounts[])
	 * [0] = rejected by REJECT table / trivial
	 * [1] = actual sight checks performed
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom Sight")
	static void GetSightCounts(int32& OutRejected, int32& OutChecked);

	/** Reset sight statistics */
	UFUNCTION(BlueprintCallable, Category = "Doom Sight")
	static void ResetSightCounts();

	// =========================================================================
	// Low-level helpers
	// =========================================================================

	/**
	 * DivLineSide: Determines which side of a divline a point is on.
	 * @return 0 = front, 1 = back, 2 = on the line
	 */
	static int32 DivLineSide(fixed_t X, fixed_t Y, fixed_t LineX, fixed_t LineY,
		fixed_t LineDX, fixed_t LineDY);

	/**
	 * InterceptVector2: Fraction along strace where it meets divl.
	 * Used to compute slope at intersection.
	 */
	static fixed_t InterceptVector2(fixed_t StraceX, fixed_t StraceY,
		fixed_t StraceDX, fixed_t StraceDY,
		fixed_t DivlX, fixed_t DivlY,
		fixed_t DivlDX, fixed_t DivlDY);

private:
	/** Cached sight counts for stats (port of sightcounts[0/1]) */
	static int32 SightCountRejected;
	static int32 SightCountChecked;
};
