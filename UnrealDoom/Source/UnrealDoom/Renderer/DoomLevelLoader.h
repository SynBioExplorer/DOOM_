#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// Level loader - reads DOOM WAD map lumps and builds runtime geometry data.
// Equivalent to p_setup.c: P_LoadVertexes, P_LoadSectors, P_LoadSideDefs,
// P_LoadLineDefs, P_LoadSubsectors, P_LoadNodes, P_LoadSegs, P_LoadBlockMap,
// P_LoadThings, P_GroupLines.

#include "CoreMinimal.h"
#include "DoomMapData.h"
#include "DoomLevelLoader.generated.h"

// Forward declarations
class UWadManager;

/**
 * UDoomLevelLoader
 *
 * Loads a complete DOOM level from WAD data into runtime TArrays of
 * map geometry structures. All fixed-point coordinates are converted
 * to Unreal float coordinates during loading.
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomLevelLoader : public UObject
{
	GENERATED_BODY()

public:
	// ====================================================================
	// Loaded map data - public so other systems can reference them
	// ====================================================================

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomVertex> Vertexes;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomSector> Sectors;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomSide> Sides;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomLine> Lines;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomSubSector> SubSectors;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomNode> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomSeg> Segs;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	FDoomBlockmap Blockmap;

	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomMapThing> Things;

	/** Reject matrix data (sector-sector visibility LUT) */
	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<uint8> RejectMatrix;

	/** Player start positions (up to 4 players) */
	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomMapThing> PlayerStarts;

	/** Deathmatch start positions */
	UPROPERTY(BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomMapThing> DeathmatchStarts;

	// ====================================================================
	// Main loading interface
	// ====================================================================

	/**
	 * Load a complete level from the WAD.
	 * For DOOM 1: Episode 1-4, Map 1-9 (E1M1 format).
	 * For DOOM 2: Episode 1, Map 1-32 (MAP01 format).
	 *
	 * @param WadManager  The WAD manager containing loaded lump data.
	 * @param Episode     Episode number (1-based, ignored for DOOM 2).
	 * @param Map         Map number (1-based).
	 * @return true if the level was loaded successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	bool LoadLevel(UWadManager* WadManager, int32 Episode, int32 Map);

	/** Check if level data is currently loaded */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	bool IsLoaded() const { return bIsLoaded; }

	/** Clear all loaded data */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	void ClearLevel();

private:
	// ====================================================================
	// Individual lump loaders (mirrors p_setup.c functions)
	// ====================================================================

	/** Load vertexes from ML_VERTEXES lump. Converts map int16 -> fixed_t -> float. */
	bool P_LoadVertexes(const TArray<uint8>& LumpData);

	/** Load sectors from ML_SECTORS lump. */
	bool P_LoadSectors(const TArray<uint8>& LumpData);

	/** Load sidedefs from ML_SIDEDEFS lump. */
	bool P_LoadSideDefs(const TArray<uint8>& LumpData);

	/** Load linedefs from ML_LINEDEFS lump. Links to vertexes and sides. */
	bool P_LoadLineDefs(const TArray<uint8>& LumpData);

	/** Load subsectors from ML_SSECTORS lump. */
	bool P_LoadSubsectors(const TArray<uint8>& LumpData);

	/** Load BSP nodes from ML_NODES lump. */
	bool P_LoadNodes(const TArray<uint8>& LumpData);

	/** Load segs from ML_SEGS lump. Links to vertexes, linedefs, sidedefs, sectors. */
	bool P_LoadSegs(const TArray<uint8>& LumpData);

	/** Load blockmap from ML_BLOCKMAP lump. */
	bool P_LoadBlockMap(const TArray<uint8>& LumpData);

	/** Load things from ML_THINGS lump. */
	bool P_LoadThings(const TArray<uint8>& LumpData);

	/**
	 * Group lines by sector - builds sector line lists and
	 * subsector sector references. Computes sector bounding boxes
	 * and sound origins. Equivalent to P_GroupLines().
	 */
	void P_GroupLines();

	// ====================================================================
	// Helpers
	// ====================================================================

	/** Convert a DOOM fixed_t value (already shifted <<16) to Unreal float units */
	static float FixedToUnreal(int32 FixedValue);

	/** Convert a raw map int16 coordinate to Unreal float units (applies <<FRACBITS then scale) */
	static float MapCoordToUnreal(int16 MapValue);

	/** Read a null-padded 8-char name from a byte buffer into an FString */
	static FString ReadName8(const uint8* Data);

	bool bIsLoaded = false;
};
