#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// ADoomLevelActor - Holds all procedurally generated geometry for a DOOM level.
// Contains the ProceduralMeshComponent used to render floors, ceilings,
// and walls, with multiple mesh sections for different textures/light levels.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoomLevelActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * FDoomMeshSection - Describes a single mesh section within the level geometry.
 * Each section corresponds to a unique texture + light level combination,
 * allowing the ProceduralMeshComponent to batch draw calls by material.
 */
USTRUCT(BlueprintType)
struct FDoomMeshSection
{
	GENERATED_BODY()

	/** Mesh section index within the ProceduralMeshComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Mesh")
	int32 SectionIndex = -1;

	/** Texture/flat name this section uses */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Mesh")
	FString TextureName;

	/** Light level (0-255) for this section */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Mesh")
	int32 LightLevel = 0;

	/** Dynamic material instance for this section */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Mesh")
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	/** Whether this section has collision enabled (walls only) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Mesh")
	bool bHasCollision = false;
};

/**
 * ADoomLevelActor - The single actor that holds all generated level geometry.
 *
 * The DoomMeshGenerator creates one of these per loaded level. It contains:
 * - A UProceduralMeshComponent with multiple sections
 * - Each section represents a unique texture + light level combination
 * - Wall sections have collision; floor/ceiling sections may not
 * - Dynamic material instances allow per-section light level control
 *
 * This actor is spawned by ADoomGameMode::LoadLevel and destroyed
 * when the level changes.
 */
UCLASS(Blueprintable, ClassGroup = "Doom")
class UNREALDOOM_API ADoomLevelActor : public AActor
{
	GENERATED_BODY()

public:
	ADoomLevelActor();

	// =========================================================================
	// Geometry building interface (called by DoomMeshGenerator)
	// =========================================================================

	/**
	 * Clear all existing geometry and prepare for a new level.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	void ClearGeometry();

	/**
	 * Create a new mesh section for a specific texture and light level.
	 * Returns the section index for later vertex/triangle data submission.
	 *
	 * @param TextureName - The DOOM texture/flat name for this section
	 * @param LightLevel - Sector light level (0-255)
	 * @param bEnableCollision - Whether this section should generate collision
	 * @return The section index
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	int32 CreateMeshSection(const FString& TextureName, int32 LightLevel, bool bEnableCollision);

	/**
	 * Set the geometry data for a mesh section.
	 * Vertex positions, normals, UVs, and triangle indices.
	 *
	 * @param SectionIndex - Index returned from CreateMeshSection
	 * @param Vertices - Vertex positions in UE world space
	 * @param Triangles - Triangle index buffer (3 indices per triangle)
	 * @param Normals - Per-vertex normals
	 * @param UVs - Per-vertex texture coordinates
	 * @param VertexColors - Per-vertex colors (for light level baking)
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	void SetSectionGeometry(
		int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UVs,
		const TArray<FColor>& VertexColors
	);

	/**
	 * Finalize all sections after geometry has been submitted.
	 * Enables collision computation and optimizes the mesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	void FinalizeGeometry();

	/**
	 * Update the light level for a specific mesh section.
	 * Used for dynamic lighting effects (strobes, glows, flickers).
	 *
	 * @param SectionIndex - The mesh section to update
	 * @param NewLightLevel - New light level (0-255)
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	void UpdateSectionLightLevel(int32 SectionIndex, int32 NewLightLevel);

	/**
	 * Set the material instance for a mesh section.
	 * @param SectionIndex - The section to update
	 * @param Material - The material to assign
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	void SetSectionMaterial(int32 SectionIndex, UMaterialInterface* Material);

	/**
	 * Find or create a mesh section for a given texture + light level pair.
	 * Reuses an existing section if one already exists.
	 *
	 * @param TextureName - The DOOM texture name
	 * @param LightLevel - Sector light level
	 * @param bEnableCollision - Whether collision is needed
	 * @return Section index
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Level")
	int32 FindOrCreateSection(const FString& TextureName, int32 LightLevel, bool bEnableCollision);

	// =========================================================================
	// Accessors
	// =========================================================================

	/** @return The procedural mesh component */
	UFUNCTION(BlueprintPure, Category = "Doom|Level")
	UProceduralMeshComponent* GetProceduralMesh() const { return ProceduralMesh; }

	/** @return Number of mesh sections */
	UFUNCTION(BlueprintPure, Category = "Doom|Level")
	int32 GetNumSections() const { return MeshSections.Num(); }

	/** @return The mesh section info at the given index */
	const FDoomMeshSection* GetMeshSection(int32 Index) const
	{
		return MeshSections.IsValidIndex(Index) ? &MeshSections[Index] : nullptr;
	}

protected:
	// =========================================================================
	// Components
	// =========================================================================

	/** The procedural mesh that holds all level geometry */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Level")
	TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

	// =========================================================================
	// Mesh section tracking
	// =========================================================================

	/** Array of all mesh sections and their metadata */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Level")
	TArray<FDoomMeshSection> MeshSections;

	/** Counter for the next section index */
	int32 NextSectionIndex = 0;

	// =========================================================================
	// Material
	// =========================================================================

	/** Base material used to create per-section dynamic instances.
	 *  Should have parameters for: Texture, LightLevel, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Level|Material")
	TObjectPtr<UMaterialInterface> BaseLevelMaterial;

	/** Lookup map from "TextureName_LightLevel" -> section index for fast reuse */
	TMap<FString, int32> SectionLookup;

private:
	/** Build the lookup key for a texture + light level pair */
	static FString MakeSectionKey(const FString& TextureName, int32 LightLevel);
};
