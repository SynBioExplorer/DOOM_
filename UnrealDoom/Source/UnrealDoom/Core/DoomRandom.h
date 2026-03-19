#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// DOOM pseudo-random number generator ported to Unreal Engine 5.
// Exact port of m_random.c - maintains identical behavior.

#include "CoreMinimal.h"

/**
 * FDoomRandom - The DOOM pseudo-random number generator.
 *
 * Uses a 256-entry lookup table (rndtable) from the original DOOM source.
 * Maintains two separate indices:
 *   - PrndIndex: used by P_Random() for gameplay/physics (deterministic for demos)
 *   - RndIndex:  used by M_Random() for non-gameplay randomness
 */
struct UNREALDOOM_API FDoomRandom
{
	/**
	 * Returns a pseudo-random number 0-255 for gameplay use.
	 * This is the deterministic random used for physics, AI, etc.
	 * Must stay in sync for demo playback and netgame consistency.
	 */
	static int32 P_Random();

	/**
	 * Returns a pseudo-random number 0-255 for non-gameplay use.
	 * Used for menus, screen effects, and other non-critical randomness.
	 */
	static int32 M_Random();

	/**
	 * Reset both random indices to 0.
	 * Called at the start of a new game/demo to ensure determinism.
	 */
	static void M_ClearRandom();

	/** Current index for M_Random */
	static int32 RndIndex;

	/** Current index for P_Random */
	static int32 PrndIndex;

	/** The 256-entry random number lookup table */
	static const uint8 RndTable[256];
};
