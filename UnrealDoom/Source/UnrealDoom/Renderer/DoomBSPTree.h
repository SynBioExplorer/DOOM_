#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// BSP tree traversal - equivalent to r_bsp.c / r_bsp.h logic.
// Stores the BSP tree from loaded nodes and provides spatial queries:
// R_PointInSubsector, TraverseBSP, GetSubsectorAt.

#include "CoreMinimal.h"
#include "DoomMapData.h"
#include "DoomBSPTree.generated.h"

// Forward declaration
class UDoomLevelLoader;

/**
 * Delegate for BSP traversal callbacks.
 * Called for each subsector visited during traversal.
 * Return true to continue traversal, false to stop.
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnVisitSubsector, int32 /* SubSectorIndex */);

/**
 * UDoomBSPTree
 *
 * Manages the BSP tree built from DOOM level data.
 * Provides R_PointInSubsector for position lookups and
 * front-to-back traversal for rendering/collision.
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomBSPTree : public UObject
{
	GENERATED_BODY()

public:
	// ====================================================================
	// Initialization
	// ====================================================================

	/**
	 * Initialize the BSP tree from loaded level data.
	 * Must be called after UDoomLevelLoader has finished loading.
	 *
	 * @param LevelLoader  The level loader containing nodes, subsectors, segs, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	void Initialize(UDoomLevelLoader* LevelLoader);

	/** Check if the BSP tree has been initialized */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	bool IsInitialized() const { return bInitialized; }

	// ====================================================================
	// Spatial queries
	// ====================================================================

	/**
	 * Find which subsector contains the given point.
	 * Equivalent to R_PointInSubsector from r_main.c.
	 *
	 * @param X  X coordinate in Unreal units.
	 * @param Y  Y coordinate in Unreal units.
	 * @return Index of the subsector containing the point, or -1 if BSP is empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	int32 R_PointInSubsector(float X, float Y) const;

	/**
	 * Get the subsector at a world position (convenience wrapper).
	 *
	 * @param Position  World position (only X and Y are used).
	 * @return Pointer to the subsector, or nullptr if not found.
	 */
	const FDoomSubSector* GetSubsectorAt(const FVector& Position) const;

	/**
	 * Get the sector index at a world position.
	 *
	 * @param X  X coordinate in Unreal units.
	 * @param Y  Y coordinate in Unreal units.
	 * @return Sector index, or -1 if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	int32 GetSectorIndexAt(float X, float Y) const;

	// ====================================================================
	// BSP traversal
	// ====================================================================

	/**
	 * Traverse the BSP tree from the given viewpoint, visiting subsectors
	 * in approximate front-to-back order (as the original R_RenderBSPNode does).
	 *
	 * @param ViewX      Viewer X position in Unreal units.
	 * @param ViewY      Viewer Y position in Unreal units.
	 * @param Callback   Called for each subsector visited. Return false to stop.
	 */
	void TraverseBSP(float ViewX, float ViewY, const FOnVisitSubsector& Callback) const;

	/**
	 * Blueprint-friendly traversal that collects all subsector indices
	 * in front-to-back order from the given viewpoint.
	 *
	 * @param ViewX      Viewer X position in Unreal units.
	 * @param ViewY      Viewer Y position in Unreal units.
	 * @param OutSubsectors  Filled with subsector indices in traversal order.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	void TraverseBSPCollect(float ViewX, float ViewY, TArray<int32>& OutSubsectors) const;

	// ====================================================================
	// Accessors
	// ====================================================================

	/** Get the number of nodes */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	int32 GetNumNodes() const { return CachedNodes.Num(); }

	/** Get the number of subsectors */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	int32 GetNumSubsectors() const { return CachedSubSectors.Num(); }

	/** Get the root node index (last node in the array) */
	UFUNCTION(BlueprintCallable, Category = "Doom|BSP")
	int32 GetRootNodeIndex() const { return CachedNodes.Num() - 1; }

private:
	// ====================================================================
	// Internal traversal
	// ====================================================================

	/**
	 * Determine which side of a BSP partition line a point is on.
	 * Equivalent to R_PointOnSide from r_main.c.
	 *
	 * @param X     Point X in Unreal units.
	 * @param Y     Point Y in Unreal units.
	 * @param Node  The BSP node defining the partition line.
	 * @return 0 for front (right) side, 1 for back (left) side.
	 */
	int32 R_PointOnSide(float X, float Y, const FDoomNode& Node) const;

	/**
	 * Recursive BSP traversal implementation.
	 * Mirrors R_RenderBSPNode from r_bsp.c.
	 *
	 * @param NodeNum   Current node index (may have NF_SUBSECTOR flag set).
	 * @param ViewX     Viewer X position.
	 * @param ViewY     Viewer Y position.
	 * @param Callback  Subsector visit callback.
	 * @return false if traversal was stopped by the callback.
	 */
	bool TraverseBSPNode(int32 NodeNum, float ViewX, float ViewY,
						 const FOnVisitSubsector& Callback) const;

	// ====================================================================
	// Cached data (references into the level loader's arrays)
	// ====================================================================

	/** Cached copy of nodes for fast traversal */
	TArray<FDoomNode> CachedNodes;

	/** Cached copy of subsectors */
	TArray<FDoomSubSector> CachedSubSectors;

	/** Cached copy of segs (needed for sector lookups via subsectors) */
	TArray<FDoomSeg> CachedSegs;

	/** Pointer to the level loader (kept alive via UPROPERTY) */
	UPROPERTY()
	TObjectPtr<UDoomLevelLoader> LevelData;

	bool bInitialized = false;
};
