#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// Mesh generator - converts DOOM BSP/sector geometry into UE5
// ProceduralMeshComponent meshes. This is the bridge between DOOM's
// data-driven level geometry and UE5's rendering pipeline.

#include "CoreMinimal.h"
#include "DoomMapData.h"
#include "ProceduralMeshComponent.h"
#include "DoomMeshGenerator.generated.h"

// Forward declarations
class UDoomLevelLoader;
class UDoomBSPTree;

/**
 * Result of a single mesh section generation.
 * Contains the raw vertex/index/UV data before being committed
 * to a ProceduralMeshComponent section.
 */
USTRUCT(BlueprintType)
struct FDoomMeshSection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	TArray<int32> Triangles;

	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	TArray<FVector> Normals;

	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	TArray<FVector2D> UV0;

	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	TArray<FColor> VertexColors;

	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	TArray<FProcMeshTangent> Tangents;

	/** Texture name associated with this section (for material assignment) */
	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	FString TextureName;

	/** Sector index this geometry belongs to */
	UPROPERTY(BlueprintReadWrite, Category = "Doom|Mesh")
	int32 SectorIndex = -1;

	bool IsValid() const { return Vertices.Num() > 0 && Triangles.Num() > 0; }

	void Reset()
	{
		Vertices.Empty();
		Triangles.Empty();
		Normals.Empty();
		UV0.Empty();
		VertexColors.Empty();
		Tangents.Empty();
	}
};

/**
 * UDoomMeshGenerator
 *
 * Converts DOOM's BSP/sector geometry into UE5 ProceduralMeshComponent meshes.
 * Handles floors, ceilings, and wall segments (upper/middle/lower) for both
 * one-sided and two-sided linedefs.
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomMeshGenerator : public UObject
{
	GENERATED_BODY()

public:
	// ====================================================================
	// Main generation interface
	// ====================================================================

	/**
	 * Generate the complete level mesh from loaded DOOM data.
	 * Creates a ProceduralMeshComponent on the specified actor with
	 * one section per distinct surface (floor, ceiling, wall segment).
	 *
	 * @param LevelData   Loaded level data from UDoomLevelLoader.
	 * @param BSPTree     Optional BSP tree for traversal-order generation.
	 * @param LevelActor  The actor to attach the ProceduralMeshComponent to.
	 * @return The created ProceduralMeshComponent, or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Mesh")
	UProceduralMeshComponent* GenerateLevelMesh(
		UDoomLevelLoader* LevelData,
		UDoomBSPTree* BSPTree,
		AActor* LevelActor);

	// ====================================================================
	// Individual geometry generators
	// ====================================================================

	/**
	 * Generate floor geometry for a sector.
	 * Triangulates the sector floor polygon from its bounding segs.
	 *
	 * @param SectorIndex  Index of the sector in the level data.
	 * @param LevelData    The loaded level data.
	 * @param OutSection   Filled with the floor mesh data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Mesh")
	void GenerateSectorFloor(int32 SectorIndex, UDoomLevelLoader* LevelData, FDoomMeshSection& OutSection);

	/**
	 * Generate ceiling geometry for a sector.
	 *
	 * @param SectorIndex  Index of the sector in the level data.
	 * @param LevelData    The loaded level data.
	 * @param OutSection   Filled with the ceiling mesh data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Mesh")
	void GenerateSectorCeiling(int32 SectorIndex, UDoomLevelLoader* LevelData, FDoomMeshSection& OutSection);

	/**
	 * Generate wall segment geometry for a seg.
	 * Handles upper, middle, and lower wall textures depending on
	 * whether the linedef is one-sided or two-sided.
	 *
	 * @param SegIndex     Index of the seg in the level data.
	 * @param LevelData    The loaded level data.
	 * @param OutSections  Filled with 0-3 mesh sections (upper, middle, lower).
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Mesh")
	void GenerateWallSegment(int32 SegIndex, UDoomLevelLoader* LevelData, TArray<FDoomMeshSection>& OutSections);

private:
	// ====================================================================
	// Wall generation helpers
	// ====================================================================

	/**
	 * Create a single wall quad between two vertices at given heights.
	 *
	 * @param V1           Bottom-left XY position.
	 * @param V2           Bottom-right XY position.
	 * @param BottomZ      Bottom Z height in Unreal units.
	 * @param TopZ         Top Z height in Unreal units.
	 * @param TexOffset    Horizontal texture offset in Unreal units.
	 * @param RowOffset    Vertical texture offset in Unreal units.
	 * @param SegOffset    Distance along linedef for this seg.
	 * @param LightLevel   Sector light level (0-255).
	 * @param TextureName  Name of the texture for this wall.
	 * @param SectorIdx    Sector index for this wall.
	 * @param OutSection   Filled with the wall quad mesh data.
	 */
	void CreateWallQuad(
		const FVector2D& V1,
		const FVector2D& V2,
		float BottomZ,
		float TopZ,
		float TexOffset,
		float RowOffset,
		float SegOffset,
		int32 LightLevel,
		const FString& TextureName,
		int32 SectorIdx,
		FDoomMeshSection& OutSection);

	// ====================================================================
	// Floor/ceiling triangulation
	// ====================================================================

	/**
	 * Collect the boundary vertices of a sector by walking its segs.
	 * Returns vertices in winding order suitable for triangulation.
	 *
	 * @param SectorIndex  Index of the sector.
	 * @param LevelData    The loaded level data.
	 * @param OutVertices  Filled with 2D vertex positions forming the sector boundary.
	 */
	void CollectSectorVertices(int32 SectorIndex, UDoomLevelLoader* LevelData,
							   TArray<FVector2D>& OutVertices);

	/**
	 * Triangulate a 2D polygon using ear-clipping.
	 *
	 * @param Polygon      2D polygon vertices in order.
	 * @param OutIndices   Filled with triangle indices (3 per triangle).
	 */
	void TriangulatePolygon(const TArray<FVector2D>& Polygon, TArray<int32>& OutIndices);

	/**
	 * Check if a point is inside a triangle (2D).
	 */
	static bool IsPointInTriangle(const FVector2D& P, const FVector2D& A,
								  const FVector2D& B, const FVector2D& C);

	/**
	 * Check if a triangle has counter-clockwise winding (2D).
	 */
	static float TriangleSign(const FVector2D& A, const FVector2D& B, const FVector2D& C);

	/**
	 * Compute vertex color from DOOM light level (0-255).
	 * DOOM sectors use light levels for ambient lighting.
	 */
	static FColor LightLevelToColor(int32 LightLevel);

	// ====================================================================
	// State
	// ====================================================================

	/** Running section index for the ProceduralMeshComponent */
	int32 CurrentSectionIndex = 0;

	/** Default texture size for UV calculation (64x64 for DOOM flats, 128/256 for walls) */
	static constexpr float DEFAULT_FLAT_SIZE = 64.0f;
	static constexpr float DEFAULT_WALL_WIDTH = 128.0f;
	static constexpr float DEFAULT_WALL_HEIGHT = 128.0f;
};
