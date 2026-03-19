#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Fixed-point math conversion utilities ported to Unreal Engine 5.

#include "CoreMinimal.h"
#include "DoomTypes.h"

// =============================================================================
// Angle constants (Binary Angle Measurement)
// =============================================================================

constexpr angle_t ANG45  = 0x20000000u;
constexpr angle_t ANG90  = 0x40000000u;
constexpr angle_t ANG180 = 0x80000000u;
constexpr angle_t ANG270 = 0xc0000000u;

// =============================================================================
// Fine angle lookup table constants
// =============================================================================

constexpr int32 FINEANGLES         = 8192;
constexpr int32 FINEMASK           = (FINEANGLES - 1);
constexpr int32 ANGLETOFINESHIFT   = 19;

// Slope constants
constexpr int32 SLOPERANGE = 2048;
constexpr int32 SLOPEBITS  = 11;
constexpr int32 DBITS      = (FRACBITS - SLOPEBITS);

// =============================================================================
// Scale factor: 1 DOOM unit = 2.0 Unreal Engine units
// =============================================================================

constexpr float DOOM_TO_UE_SCALE = 2.0f;

// =============================================================================
// Sine/Cosine/Tangent lookup tables
// Effective size of finesine is 10240 (5*FINEANGLES/4).
// finecosine is a pointer into finesine offset by FINEANGLES/4 (2048).
// finetangent has FINEANGLES/2 (4096) entries.
// =============================================================================

/** Fine sine table: 10240 entries (5*8192/4), values in 16.16 fixed-point */
extern fixed_t GFineSine[5 * FINEANGLES / 4];

/** Fine cosine: points to GFineSine[FINEANGLES/4] */
extern fixed_t* GFineCosine;

/** Fine tangent table: 4096 entries */
extern fixed_t GFineTangent[FINEANGLES / 2];

/** Tangent to angle lookup: 2049 entries */
extern angle_t GTanToAngle[SLOPERANGE + 1];

// =============================================================================
// DoomMath - Fixed-point math and coordinate conversion utilities
// =============================================================================

struct UNREALDOOM_API FDoomMath
{
	// =========================================================================
	// Fixed-point conversion
	// =========================================================================

	/** Convert DOOM fixed-point (16.16) to floating-point */
	static FORCEINLINE float FixedToFloat(fixed_t Value)
	{
		return static_cast<float>(Value) / static_cast<float>(FRACUNIT);
	}

	/** Convert floating-point to DOOM fixed-point (16.16) */
	static FORCEINLINE fixed_t FloatToFixed(float Value)
	{
		return static_cast<fixed_t>(Value * static_cast<float>(FRACUNIT));
	}

	// =========================================================================
	// Fixed-point arithmetic (exact DOOM behavior)
	// =========================================================================

	/** Fixed-point multiplication: (a * b) >> 16 */
	static FORCEINLINE fixed_t FixedMul(fixed_t a, fixed_t b)
	{
		return static_cast<fixed_t>((static_cast<int64>(a) * static_cast<int64>(b)) >> FRACBITS);
	}

	/** Fixed-point division with overflow protection */
	static fixed_t FixedDiv(fixed_t a, fixed_t b);

	// =========================================================================
	// Angle conversion
	// =========================================================================

	/** Convert DOOM binary angle to degrees (float) */
	static FORCEINLINE float AngleToFloat(angle_t Angle)
	{
		return static_cast<float>(Angle) / static_cast<float>(ANG180) * 180.0f;
	}

	/** Convert degrees (float) to DOOM binary angle */
	static FORCEINLINE angle_t FloatToAngle(float Degrees)
	{
		return static_cast<angle_t>(Degrees / 180.0f * static_cast<float>(ANG180));
	}

	// =========================================================================
	// DOOM <-> Unreal coordinate conversion
	// =========================================================================

	/** Convert DOOM units (fixed-point) to Unreal units (float) */
	static FORCEINLINE float DoomUnitsToUE(fixed_t Value)
	{
		return FixedToFloat(Value) * DOOM_TO_UE_SCALE;
	}

	/** Convert Unreal units (float) to DOOM units (fixed-point) */
	static FORCEINLINE fixed_t UEToDoomUnits(float Value)
	{
		return FloatToFixed(Value / DOOM_TO_UE_SCALE);
	}

	/**
	 * Convert DOOM position (fixed-point) to Unreal FVector.
	 * Coordinate system conversion:
	 *   DOOM Y -> UE X (forward)
	 *   DOOM X -> UE Y (right)
	 *   DOOM Z -> UE Z (up)
	 */
	static FORCEINLINE FVector DoomPosToUE(fixed_t X, fixed_t Y, fixed_t Z)
	{
		return FVector(
			DoomUnitsToUE(Y),   // DOOM Y -> UE X
			DoomUnitsToUE(X),   // DOOM X -> UE Y
			DoomUnitsToUE(Z)    // DOOM Z -> UE Z
		);
	}

	// =========================================================================
	// Table lookup helpers
	// =========================================================================

	/** Slope division utility, used by R_PointToAngle */
	static int32 SlopeDiv(uint32 Num, uint32 Den);

	/** Initialize all lookup tables (sine, cosine, tangent, tantoangle) */
	static void InitTables();
};
