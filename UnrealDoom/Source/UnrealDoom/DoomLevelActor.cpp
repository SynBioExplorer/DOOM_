// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.

#include "DoomLevelActor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomLevel, Log, All);

// =============================================================================
// Constructor
// =============================================================================

ADoomLevelActor::ADoomLevelActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create the procedural mesh component as root
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	ProceduralMesh->bUseAsyncCooking = true;
	ProceduralMesh->SetCastShadow(false);
	// Enable complex collision for precise wall collision
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->SetCollisionObjectType(ECC_WorldStatic);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SetRootComponent(ProceduralMesh);
}

// =============================================================================
// Geometry building interface
// =============================================================================

void ADoomLevelActor::ClearGeometry()
{
	UE_LOG(LogDoomLevel, Log, TEXT("Clearing level geometry"));

	if (ProceduralMesh)
	{
		ProceduralMesh->ClearAllMeshSections();
	}

	MeshSections.Empty();
	SectionLookup.Empty();
	NextSectionIndex = 0;
}

int32 ADoomLevelActor::CreateMeshSection(const FString& TextureName, int32 LightLevel, bool bEnableCollision)
{
	const int32 SectionIdx = NextSectionIndex++;

	FDoomMeshSection NewSection;
	NewSection.SectionIndex = SectionIdx;
	NewSection.TextureName = TextureName;
	NewSection.LightLevel = FMath::Clamp(LightLevel, 0, 255);
	NewSection.bHasCollision = bEnableCollision;

	// Create a dynamic material instance if we have a base material
	if (BaseLevelMaterial)
	{
		NewSection.MaterialInstance = UMaterialInstanceDynamic::Create(BaseLevelMaterial, this);
		if (NewSection.MaterialInstance)
		{
			// Set the light level as a scalar parameter (0.0 to 1.0)
			const float NormalizedLight = static_cast<float>(NewSection.LightLevel) / 255.0f;
			NewSection.MaterialInstance->SetScalarParameterValue(TEXT("LightLevel"), NormalizedLight);
		}
	}

	MeshSections.Add(MoveTemp(NewSection));

	// Add to lookup table
	const FString Key = MakeSectionKey(TextureName, LightLevel);
	SectionLookup.Add(Key, SectionIdx);

	UE_LOG(LogDoomLevel, Verbose, TEXT("Created mesh section %d: Texture=%s Light=%d Collision=%d"),
		SectionIdx, *TextureName, LightLevel, bEnableCollision ? 1 : 0);

	return SectionIdx;
}

void ADoomLevelActor::SetSectionGeometry(
	int32 SectionIndex,
	const TArray<FVector>& Vertices,
	const TArray<int32>& Triangles,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UVs,
	const TArray<FColor>& VertexColors)
{
	if (!ProceduralMesh)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("ProceduralMesh is null"));
		return;
	}

	if (!MeshSections.IsValidIndex(SectionIndex))
	{
		UE_LOG(LogDoomLevel, Error, TEXT("Invalid section index: %d"), SectionIndex);
		return;
	}

	// Convert FColor array to FLinearColor for the procedural mesh
	TArray<FLinearColor> LinearColors;
	LinearColors.Reserve(VertexColors.Num());
	for (const FColor& Color : VertexColors)
	{
		LinearColors.Add(FLinearColor(Color));
	}

	// Create the mesh section
	// Empty tangent array - ProceduralMeshComponent will calculate tangents
	TArray<FProcMeshTangent> Tangents;

	ProceduralMesh->CreateMeshSection_LinearColor(
		SectionIndex,
		Vertices,
		Triangles,
		Normals,
		UVs,
		LinearColors,
		Tangents,
		MeshSections[SectionIndex].bHasCollision
	);

	// Apply the material instance if available
	const FDoomMeshSection& Section = MeshSections[SectionIndex];
	if (Section.MaterialInstance)
	{
		ProceduralMesh->SetMaterial(SectionIndex, Section.MaterialInstance);
	}

	UE_LOG(LogDoomLevel, Verbose, TEXT("Set geometry for section %d: %d verts, %d tris"),
		SectionIndex, Vertices.Num(), Triangles.Num() / 3);
}

void ADoomLevelActor::FinalizeGeometry()
{
	UE_LOG(LogDoomLevel, Log, TEXT("Finalizing level geometry: %d sections"), MeshSections.Num());

	// The ProceduralMeshComponent automatically computes collision
	// when sections are created with bHasCollision=true.
	// No additional finalization needed for now, but this is where
	// we could do post-processing like:
	// - Merging adjacent sections with the same material
	// - Building a simplified collision mesh
	// - Computing ambient occlusion or lightmap data

	// Log statistics
	int32 TotalVerts = 0;
	int32 TotalTris = 0;
	int32 CollisionSections = 0;

	for (const FDoomMeshSection& Section : MeshSections)
	{
		if (Section.bHasCollision)
		{
			CollisionSections++;
		}
	}

	UE_LOG(LogDoomLevel, Log, TEXT("Level geometry finalized: %d total sections, %d with collision"),
		MeshSections.Num(), CollisionSections);
}

void ADoomLevelActor::UpdateSectionLightLevel(int32 SectionIndex, int32 NewLightLevel)
{
	if (!MeshSections.IsValidIndex(SectionIndex))
	{
		return;
	}

	FDoomMeshSection& Section = MeshSections[SectionIndex];
	Section.LightLevel = FMath::Clamp(NewLightLevel, 0, 255);

	// Update the dynamic material instance
	if (Section.MaterialInstance)
	{
		const float NormalizedLight = static_cast<float>(Section.LightLevel) / 255.0f;
		Section.MaterialInstance->SetScalarParameterValue(TEXT("LightLevel"), NormalizedLight);
	}
}

void ADoomLevelActor::SetSectionMaterial(int32 SectionIndex, UMaterialInterface* Material)
{
	if (!ProceduralMesh || !MeshSections.IsValidIndex(SectionIndex))
	{
		return;
	}

	ProceduralMesh->SetMaterial(SectionIndex, Material);
}

int32 ADoomLevelActor::FindOrCreateSection(const FString& TextureName, int32 LightLevel, bool bEnableCollision)
{
	const FString Key = MakeSectionKey(TextureName, LightLevel);

	if (const int32* ExistingIndex = SectionLookup.Find(Key))
	{
		return *ExistingIndex;
	}

	return CreateMeshSection(TextureName, LightLevel, bEnableCollision);
}

// =============================================================================
// Private helpers
// =============================================================================

FString ADoomLevelActor::MakeSectionKey(const FString& TextureName, int32 LightLevel)
{
	return FString::Printf(TEXT("%s_%d"), *TextureName, LightLevel);
}
