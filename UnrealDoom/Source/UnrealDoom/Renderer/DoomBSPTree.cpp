// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// BSP tree traversal implementation - mirrors r_bsp.c logic.

#include "DoomBSPTree.h"
#include "DoomLevelLoader.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomBSP, Log, All);

// ============================================================================
// Initialize
// ============================================================================

void UDoomBSPTree::Initialize(UDoomLevelLoader* LevelLoader)
{
	bInitialized = false;

	if (!LevelLoader)
	{
		UE_LOG(LogDoomBSP, Error, TEXT("Initialize: LevelLoader is null"));
		return;
	}

	LevelData = LevelLoader;

	// Cache the data locally for fast traversal
	CachedNodes = LevelLoader->Nodes;
	CachedSubSectors = LevelLoader->SubSectors;
	CachedSegs = LevelLoader->Segs;

	if (CachedNodes.Num() == 0)
	{
		UE_LOG(LogDoomBSP, Warning, TEXT("Initialize: No BSP nodes loaded"));
		// A map with zero nodes but one subsector is technically valid (single convex room)
	}

	bInitialized = true;

	UE_LOG(LogDoomBSP, Log, TEXT("Initialize: BSP tree ready (%d nodes, %d subsectors)"),
		CachedNodes.Num(), CachedSubSectors.Num());
}

// ============================================================================
// R_PointOnSide - determine which side of a partition line a point is on
// Mirrors R_PointOnSide from r_main.c
// ============================================================================

int32 UDoomBSPTree::R_PointOnSide(float X, float Y, const FDoomNode& Node) const
{
	// The partition line goes from (Node.X, Node.Y) in direction (Node.Dx, Node.Dy).
	// We compute the cross product to determine which side the point is on.
	const float DeltaX = X - Node.X;
	const float DeltaY = Y - Node.Y;

	// Cross product: Dx * DeltaY - Dy * DeltaX
	// If positive, point is on the right (front) side -> return 0
	// If negative, point is on the left (back) side -> return 1
	const float Cross = Node.Dx * DeltaY - Node.Dy * DeltaX;

	// Original DOOM returns 0 for front (right) side, 1 for back (left) side
	if (Cross >= 0.0f)
	{
		return 0; // Front / right side
	}

	return 1; // Back / left side
}

// ============================================================================
// R_PointInSubsector - find which subsector contains a point
// Mirrors R_PointInSubsector from r_main.c
// ============================================================================

int32 UDoomBSPTree::R_PointInSubsector(float X, float Y) const
{
	if (!bInitialized)
	{
		return -1;
	}

	// Special case: no nodes means the entire map is one subsector
	if (CachedNodes.Num() == 0)
	{
		return (CachedSubSectors.Num() > 0) ? 0 : -1;
	}

	// Start at the root node (last node in the array)
	int32 NodeNum = CachedNodes.Num() - 1;

	while (true)
	{
		// Check if we have reached a subsector (leaf)
		if (NodeNum & NF_SUBSECTOR)
		{
			// Special case from original: nodenum == -1 means subsector 0
			if (NodeNum == -1)
			{
				return 0;
			}

			const int32 SubSectorIdx = NodeNum & (~NF_SUBSECTOR);
			if (SubSectorIdx >= 0 && SubSectorIdx < CachedSubSectors.Num())
			{
				return SubSectorIdx;
			}
			return -1;
		}

		// Bounds check
		if (NodeNum < 0 || NodeNum >= CachedNodes.Num())
		{
			UE_LOG(LogDoomBSP, Warning, TEXT("R_PointInSubsector: Invalid node index %d"), NodeNum);
			return -1;
		}

		const FDoomNode& Node = CachedNodes[NodeNum];
		const int32 Side = R_PointOnSide(X, Y, Node);

		NodeNum = Node.Children[Side];
	}
}

// ============================================================================
// GetSubsectorAt - convenience wrapper
// ============================================================================

const FDoomSubSector* UDoomBSPTree::GetSubsectorAt(const FVector& Position) const
{
	const int32 Index = R_PointInSubsector(Position.X, Position.Y);
	if (Index >= 0 && Index < CachedSubSectors.Num())
	{
		return &CachedSubSectors[Index];
	}
	return nullptr;
}

// ============================================================================
// GetSectorIndexAt
// ============================================================================

int32 UDoomBSPTree::GetSectorIndexAt(float X, float Y) const
{
	const int32 SSIndex = R_PointInSubsector(X, Y);
	if (SSIndex >= 0 && SSIndex < CachedSubSectors.Num())
	{
		return CachedSubSectors[SSIndex].SectorIndex;
	}
	return -1;
}

// ============================================================================
// TraverseBSPNode - recursive BSP traversal
// Mirrors R_RenderBSPNode from r_bsp.c
// ============================================================================

bool UDoomBSPTree::TraverseBSPNode(int32 NodeNum, float ViewX, float ViewY,
								   const FOnVisitSubsector& Callback) const
{
	// Found a subsector?
	if (NodeNum & NF_SUBSECTOR)
	{
		int32 SubSectorIdx;
		if (NodeNum == -1)
		{
			SubSectorIdx = 0;
		}
		else
		{
			SubSectorIdx = NodeNum & (~NF_SUBSECTOR);
		}

		if (SubSectorIdx >= 0 && SubSectorIdx < CachedSubSectors.Num())
		{
			// Visit this subsector
			if (Callback.IsBound())
			{
				return Callback.Execute(SubSectorIdx);
			}
		}
		return true;
	}

	// Bounds check
	if (NodeNum < 0 || NodeNum >= CachedNodes.Num())
	{
		return true;
	}

	const FDoomNode& Node = CachedNodes[NodeNum];

	// Decide which side the view point is on
	const int32 Side = R_PointOnSide(ViewX, ViewY, Node);

	// Recursively traverse the front (near) side first
	if (!TraverseBSPNode(Node.Children[Side], ViewX, ViewY, Callback))
	{
		return false; // Callback requested stop
	}

	// Then traverse the back (far) side
	// In the original, R_CheckBBox is used here to cull invisible nodes.
	// For a general-purpose traversal, we visit everything.
	// Rendering-specific culling should be done in the callback.
	if (!TraverseBSPNode(Node.Children[Side ^ 1], ViewX, ViewY, Callback))
	{
		return false;
	}

	return true;
}

// ============================================================================
// TraverseBSP - public traversal interface
// ============================================================================

void UDoomBSPTree::TraverseBSP(float ViewX, float ViewY, const FOnVisitSubsector& Callback) const
{
	if (!bInitialized || CachedNodes.Num() == 0)
	{
		// No nodes - visit the single subsector if it exists
		if (CachedSubSectors.Num() > 0 && Callback.IsBound())
		{
			Callback.Execute(0);
		}
		return;
	}

	// Start from root (last node)
	const int32 RootNode = CachedNodes.Num() - 1;
	TraverseBSPNode(RootNode, ViewX, ViewY, Callback);
}

// ============================================================================
// TraverseBSPCollect - Blueprint-friendly traversal
// ============================================================================

void UDoomBSPTree::TraverseBSPCollect(float ViewX, float ViewY, TArray<int32>& OutSubsectors) const
{
	OutSubsectors.Empty();

	FOnVisitSubsector Collector;
	Collector.BindLambda([&OutSubsectors](int32 SubSectorIndex) -> bool
	{
		OutSubsectors.Add(SubSectorIndex);
		return true; // Continue traversal
	});

	TraverseBSP(ViewX, ViewY, Collector);
}
