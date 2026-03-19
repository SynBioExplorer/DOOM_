// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// Mesh generator implementation - converts DOOM geometry to UE5
// ProceduralMeshComponent meshes.

#include "DoomMeshGenerator.h"
#include "DoomLevelLoader.h"
#include "DoomBSPTree.h"
#include "ProceduralMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomMesh, Log, All);

// Line flag for two-sided detection
static constexpr int32 ML_TWOSIDED_FLAG = 4;

// ============================================================================
// LightLevelToColor - convert DOOM light level to vertex color
// ============================================================================

FColor UDoomMeshGenerator::LightLevelToColor(int32 LightLevel)
{
	const uint8 Brightness = static_cast<uint8>(FMath::Clamp(LightLevel, 0, 255));
	return FColor(Brightness, Brightness, Brightness, 255);
}

// ============================================================================
// GenerateLevelMesh - main entry point
// ============================================================================

UProceduralMeshComponent* UDoomMeshGenerator::GenerateLevelMesh(
	UDoomLevelLoader* LevelData,
	UDoomBSPTree* BSPTree,
	AActor* LevelActor)
{
	if (!LevelData || !LevelActor)
	{
		UE_LOG(LogDoomMesh, Error, TEXT("GenerateLevelMesh: Null LevelData or LevelActor"));
		return nullptr;
	}

	if (LevelData->Sectors.Num() == 0)
	{
		UE_LOG(LogDoomMesh, Error, TEXT("GenerateLevelMesh: No sectors loaded"));
		return nullptr;
	}

	// Create the ProceduralMeshComponent
	UProceduralMeshComponent* MeshComp = NewObject<UProceduralMeshComponent>(
		LevelActor, UProceduralMeshComponent::StaticClass(),
		FName(TEXT("DoomLevelMesh")));

	if (!MeshComp)
	{
		UE_LOG(LogDoomMesh, Error, TEXT("GenerateLevelMesh: Failed to create ProceduralMeshComponent"));
		return nullptr;
	}

	MeshComp->RegisterComponent();
	MeshComp->AttachToComponent(LevelActor->GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform);

	// Enable collision on the mesh
	MeshComp->bUseComplexAsSimpleCollision = true;

	CurrentSectionIndex = 0;

	// Generate floors and ceilings for each sector
	for (int32 i = 0; i < LevelData->Sectors.Num(); i++)
	{
		// Floor
		FDoomMeshSection FloorSection;
		GenerateSectorFloor(i, LevelData, FloorSection);
		if (FloorSection.IsValid())
		{
			MeshComp->CreateMeshSection_LinearColor(
				CurrentSectionIndex,
				FloorSection.Vertices,
				FloorSection.Triangles,
				FloorSection.Normals,
				FloorSection.UV0,
				TArray<FLinearColor>(), // Use vertex colors via the FColor overload below
				FloorSection.Tangents,
				true // bCreateCollision
			);
			// Apply vertex colors separately
			if (FloorSection.VertexColors.Num() > 0)
			{
				MeshComp->SetMeshSectionVisible(CurrentSectionIndex, true);
			}
			CurrentSectionIndex++;
		}

		// Ceiling
		FDoomMeshSection CeilingSection;
		GenerateSectorCeiling(i, LevelData, CeilingSection);
		if (CeilingSection.IsValid())
		{
			MeshComp->CreateMeshSection_LinearColor(
				CurrentSectionIndex,
				CeilingSection.Vertices,
				CeilingSection.Triangles,
				CeilingSection.Normals,
				CeilingSection.UV0,
				TArray<FLinearColor>(),
				CeilingSection.Tangents,
				false // Ceilings typically don't need collision
			);
			CurrentSectionIndex++;
		}
	}

	// Generate wall segments for each seg
	for (int32 i = 0; i < LevelData->Segs.Num(); i++)
	{
		TArray<FDoomMeshSection> WallSections;
		GenerateWallSegment(i, LevelData, WallSections);

		for (const FDoomMeshSection& Section : WallSections)
		{
			if (Section.IsValid())
			{
				MeshComp->CreateMeshSection_LinearColor(
					CurrentSectionIndex,
					Section.Vertices,
					Section.Triangles,
					Section.Normals,
					Section.UV0,
					TArray<FLinearColor>(),
					Section.Tangents,
					true // Walls need collision
				);
				CurrentSectionIndex++;
			}
		}
	}

	UE_LOG(LogDoomMesh, Log,
		TEXT("GenerateLevelMesh: Created %d mesh sections for %d sectors and %d segs"),
		CurrentSectionIndex, LevelData->Sectors.Num(), LevelData->Segs.Num());

	return MeshComp;
}

// ============================================================================
// CollectSectorVertices - gather boundary vertices for a sector
// ============================================================================

void UDoomMeshGenerator::CollectSectorVertices(int32 SectorIndex, UDoomLevelLoader* LevelData,
											   TArray<FVector2D>& OutVertices)
{
	OutVertices.Empty();

	if (!LevelData || SectorIndex < 0 || SectorIndex >= LevelData->Sectors.Num())
	{
		return;
	}

	// Collect all unique vertices from segs belonging to subsectors of this sector.
	// We walk through all subsectors looking for ones belonging to our sector,
	// then collect their seg vertices.
	TArray<FVector2D> AllSegVertices;
	TSet<int32> VisitedVertices;

	for (int32 SSIdx = 0; SSIdx < LevelData->SubSectors.Num(); SSIdx++)
	{
		const FDoomSubSector& SS = LevelData->SubSectors[SSIdx];
		if (SS.SectorIndex != SectorIndex)
		{
			continue;
		}

		// Walk the segs for this subsector and collect vertices in order
		for (int32 SegOfs = 0; SegOfs < SS.NumLines; SegOfs++)
		{
			const int32 SegIdx = SS.FirstLine + SegOfs;
			if (SegIdx < 0 || SegIdx >= LevelData->Segs.Num())
			{
				continue;
			}

			const FDoomSeg& Seg = LevelData->Segs[SegIdx];

			if (Seg.V1 >= 0 && Seg.V1 < LevelData->Vertexes.Num())
			{
				const FDoomVertex& V = LevelData->Vertexes[Seg.V1];
				AllSegVertices.Add(FVector2D(V.X, V.Y));
			}
		}
	}

	if (AllSegVertices.Num() < 3)
	{
		return;
	}

	// Remove duplicate vertices (within tolerance) while preserving order
	const float Tolerance = 0.1f;
	OutVertices.Add(AllSegVertices[0]);
	for (int32 i = 1; i < AllSegVertices.Num(); i++)
	{
		bool bDuplicate = false;
		for (const FVector2D& Existing : OutVertices)
		{
			if (FVector2D::DistSquared(AllSegVertices[i], Existing) < Tolerance * Tolerance)
			{
				bDuplicate = true;
				break;
			}
		}
		if (!bDuplicate)
		{
			OutVertices.Add(AllSegVertices[i]);
		}
	}
}

// ============================================================================
// TriangulatePolygon - ear-clipping triangulation for sector flats
// ============================================================================

float UDoomMeshGenerator::TriangleSign(const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	return (A.X - C.X) * (B.Y - C.Y) - (B.X - C.X) * (A.Y - C.Y);
}

bool UDoomMeshGenerator::IsPointInTriangle(const FVector2D& P, const FVector2D& A,
										   const FVector2D& B, const FVector2D& C)
{
	const float D1 = TriangleSign(P, A, B);
	const float D2 = TriangleSign(P, B, C);
	const float D3 = TriangleSign(P, C, A);

	const bool bHasNeg = (D1 < 0) || (D2 < 0) || (D3 < 0);
	const bool bHasPos = (D1 > 0) || (D2 > 0) || (D3 > 0);

	return !(bHasNeg && bHasPos);
}

void UDoomMeshGenerator::TriangulatePolygon(const TArray<FVector2D>& Polygon, TArray<int32>& OutIndices)
{
	OutIndices.Empty();

	if (Polygon.Num() < 3)
	{
		return;
	}

	// Build an index list for the polygon
	TArray<int32> Remaining;
	Remaining.Reserve(Polygon.Num());

	// Determine winding order - ensure counter-clockwise for correct normals
	float SignedArea = 0.0f;
	for (int32 i = 0; i < Polygon.Num(); i++)
	{
		const int32 j = (i + 1) % Polygon.Num();
		SignedArea += Polygon[i].X * Polygon[j].Y;
		SignedArea -= Polygon[j].X * Polygon[i].Y;
	}

	// If clockwise, reverse the winding
	if (SignedArea > 0.0f)
	{
		for (int32 i = Polygon.Num() - 1; i >= 0; i--)
		{
			Remaining.Add(i);
		}
	}
	else
	{
		for (int32 i = 0; i < Polygon.Num(); i++)
		{
			Remaining.Add(i);
		}
	}

	// Ear-clipping
	int32 SafetyCounter = Remaining.Num() * Remaining.Num();
	while (Remaining.Num() > 2 && SafetyCounter > 0)
	{
		bool bFoundEar = false;

		for (int32 i = 0; i < Remaining.Num(); i++)
		{
			const int32 PrevIdx = (i + Remaining.Num() - 1) % Remaining.Num();
			const int32 NextIdx = (i + 1) % Remaining.Num();

			const FVector2D& A = Polygon[Remaining[PrevIdx]];
			const FVector2D& B = Polygon[Remaining[i]];
			const FVector2D& C = Polygon[Remaining[NextIdx]];

			// Check if this is a convex vertex (ear candidate)
			const float Cross = TriangleSign(A, B, C);
			if (Cross >= 0.0f)
			{
				continue; // Reflex vertex, not an ear
			}

			// Check that no other vertex is inside this triangle
			bool bIsEar = true;
			for (int32 j = 0; j < Remaining.Num(); j++)
			{
				if (j == PrevIdx || j == i || j == NextIdx)
				{
					continue;
				}

				if (IsPointInTriangle(Polygon[Remaining[j]], A, B, C))
				{
					bIsEar = false;
					break;
				}
			}

			if (bIsEar)
			{
				// Emit triangle
				OutIndices.Add(Remaining[PrevIdx]);
				OutIndices.Add(Remaining[i]);
				OutIndices.Add(Remaining[NextIdx]);

				// Remove the ear vertex
				Remaining.RemoveAt(i);
				bFoundEar = true;
				break;
			}

			SafetyCounter--;
		}

		if (!bFoundEar)
		{
			// Degenerate polygon - break to avoid infinite loop
			UE_LOG(LogDoomMesh, Warning,
				TEXT("TriangulatePolygon: Could not find ear, %d vertices remaining"),
				Remaining.Num());
			break;
		}
	}
}

// ============================================================================
// GenerateSectorFloor
// ============================================================================

void UDoomMeshGenerator::GenerateSectorFloor(int32 SectorIndex, UDoomLevelLoader* LevelData,
											 FDoomMeshSection& OutSection)
{
	OutSection.Reset();

	if (!LevelData || SectorIndex < 0 || SectorIndex >= LevelData->Sectors.Num())
	{
		return;
	}

	const FDoomSector& Sector = LevelData->Sectors[SectorIndex];

	// Collect boundary vertices
	TArray<FVector2D> Boundary;
	CollectSectorVertices(SectorIndex, LevelData, Boundary);

	if (Boundary.Num() < 3)
	{
		return;
	}

	// Triangulate
	TArray<int32> TriIndices;
	TriangulatePolygon(Boundary, TriIndices);

	if (TriIndices.Num() < 3)
	{
		return;
	}

	// Build mesh data
	const float FloorZ = Sector.FloorHeight;
	const FColor LightColor = LightLevelToColor(Sector.LightLevel);
	const FVector FloorNormal(0.0f, 0.0f, 1.0f); // Floor faces up

	OutSection.Vertices.SetNum(Boundary.Num());
	OutSection.Normals.SetNum(Boundary.Num());
	OutSection.UV0.SetNum(Boundary.Num());
	OutSection.VertexColors.SetNum(Boundary.Num());
	OutSection.Tangents.SetNum(Boundary.Num());

	// DOOM flat texture scale: 64x64 pixels mapped to 64x64 map units
	// In Unreal units with our scale: 64 * DOOM_TO_UNREAL_SCALE = 128
	const float FlatTexScale = DEFAULT_FLAT_SIZE * DOOM_TO_UNREAL_SCALE;

	for (int32 i = 0; i < Boundary.Num(); i++)
	{
		// Position: X/Y from boundary, Z from floor height
		OutSection.Vertices[i] = FVector(Boundary[i].X, Boundary[i].Y, FloorZ);
		OutSection.Normals[i] = FloorNormal;

		// UV: tile based on world position (DOOM flats are always world-aligned)
		OutSection.UV0[i] = FVector2D(
			Boundary[i].X / FlatTexScale,
			Boundary[i].Y / FlatTexScale
		);

		OutSection.VertexColors[i] = LightColor;
		OutSection.Tangents[i] = FProcMeshTangent(1.0f, 0.0f, 0.0f);
	}

	OutSection.Triangles = TriIndices;
	OutSection.TextureName = Sector.FloorTextureName;
	OutSection.SectorIndex = SectorIndex;
}

// ============================================================================
// GenerateSectorCeiling
// ============================================================================

void UDoomMeshGenerator::GenerateSectorCeiling(int32 SectorIndex, UDoomLevelLoader* LevelData,
											   FDoomMeshSection& OutSection)
{
	OutSection.Reset();

	if (!LevelData || SectorIndex < 0 || SectorIndex >= LevelData->Sectors.Num())
	{
		return;
	}

	const FDoomSector& Sector = LevelData->Sectors[SectorIndex];

	// Collect boundary vertices (same as floor)
	TArray<FVector2D> Boundary;
	CollectSectorVertices(SectorIndex, LevelData, Boundary);

	if (Boundary.Num() < 3)
	{
		return;
	}

	// Triangulate
	TArray<int32> TriIndices;
	TriangulatePolygon(Boundary, TriIndices);

	if (TriIndices.Num() < 3)
	{
		return;
	}

	// Build mesh data - ceiling faces DOWN, so reverse winding
	const float CeilingZ = Sector.CeilingHeight;
	const FColor LightColor = LightLevelToColor(Sector.LightLevel);
	const FVector CeilingNormal(0.0f, 0.0f, -1.0f); // Ceiling faces down

	const float FlatTexScale = DEFAULT_FLAT_SIZE * DOOM_TO_UNREAL_SCALE;

	OutSection.Vertices.SetNum(Boundary.Num());
	OutSection.Normals.SetNum(Boundary.Num());
	OutSection.UV0.SetNum(Boundary.Num());
	OutSection.VertexColors.SetNum(Boundary.Num());
	OutSection.Tangents.SetNum(Boundary.Num());

	for (int32 i = 0; i < Boundary.Num(); i++)
	{
		OutSection.Vertices[i] = FVector(Boundary[i].X, Boundary[i].Y, CeilingZ);
		OutSection.Normals[i] = CeilingNormal;

		OutSection.UV0[i] = FVector2D(
			Boundary[i].X / FlatTexScale,
			Boundary[i].Y / FlatTexScale
		);

		OutSection.VertexColors[i] = LightColor;
		OutSection.Tangents[i] = FProcMeshTangent(1.0f, 0.0f, 0.0f);
	}

	// Reverse triangle winding for ceiling (faces downward)
	OutSection.Triangles.SetNum(TriIndices.Num());
	for (int32 i = 0; i < TriIndices.Num(); i += 3)
	{
		OutSection.Triangles[i + 0] = TriIndices[i + 0];
		OutSection.Triangles[i + 1] = TriIndices[i + 2]; // Swapped
		OutSection.Triangles[i + 2] = TriIndices[i + 1]; // Swapped
	}

	OutSection.TextureName = Sector.CeilingTextureName;
	OutSection.SectorIndex = SectorIndex;
}

// ============================================================================
// CreateWallQuad - build a single wall quad
// ============================================================================

void UDoomMeshGenerator::CreateWallQuad(
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
	FDoomMeshSection& OutSection)
{
	OutSection.Reset();

	if (TopZ <= BottomZ)
	{
		return; // Zero-height wall
	}

	const FColor LightColor = LightLevelToColor(LightLevel);

	// Wall normal: perpendicular to the wall direction, pointing into the sector
	const FVector2D WallDir = (V2 - V1).GetSafeNormal();
	const FVector WallNormal(-WallDir.Y, WallDir.X, 0.0f);

	const FProcMeshTangent WallTangent(WallDir.X, WallDir.Y, 0.0f);

	// Wall length for UV calculation
	const float WallLength = FVector2D::Distance(V1, V2);
	const float WallHeight = TopZ - BottomZ;

	// UV scale: DOOM textures tile based on world units
	// Horizontal: wall length / texture width (in Unreal units)
	// Vertical: wall height / texture height (in Unreal units)
	const float TexWidthUnreal = DEFAULT_WALL_WIDTH * DOOM_TO_UNREAL_SCALE;
	const float TexHeightUnreal = DEFAULT_WALL_HEIGHT * DOOM_TO_UNREAL_SCALE;

	// Base U offset comes from seg offset + sidedef texture offset
	const float BaseU = (SegOffset + TexOffset) / TexWidthUnreal;
	const float BaseV = RowOffset / TexHeightUnreal;

	// Four vertices: bottom-left, bottom-right, top-right, top-left
	// (when looking at the wall from the front)
	OutSection.Vertices.SetNum(4);
	OutSection.Vertices[0] = FVector(V1.X, V1.Y, BottomZ);  // Bottom-left
	OutSection.Vertices[1] = FVector(V2.X, V2.Y, BottomZ);  // Bottom-right
	OutSection.Vertices[2] = FVector(V2.X, V2.Y, TopZ);     // Top-right
	OutSection.Vertices[3] = FVector(V1.X, V1.Y, TopZ);     // Top-left

	// Normals (all face the same direction)
	OutSection.Normals.SetNum(4);
	OutSection.Normals[0] = WallNormal;
	OutSection.Normals[1] = WallNormal;
	OutSection.Normals[2] = WallNormal;
	OutSection.Normals[3] = WallNormal;

	// UVs
	OutSection.UV0.SetNum(4);
	OutSection.UV0[0] = FVector2D(BaseU, BaseV + WallHeight / TexHeightUnreal);               // Bottom-left
	OutSection.UV0[1] = FVector2D(BaseU + WallLength / TexWidthUnreal, BaseV + WallHeight / TexHeightUnreal); // Bottom-right
	OutSection.UV0[2] = FVector2D(BaseU + WallLength / TexWidthUnreal, BaseV);                 // Top-right
	OutSection.UV0[3] = FVector2D(BaseU, BaseV);                                               // Top-left

	// Vertex colors
	OutSection.VertexColors.SetNum(4);
	OutSection.VertexColors[0] = LightColor;
	OutSection.VertexColors[1] = LightColor;
	OutSection.VertexColors[2] = LightColor;
	OutSection.VertexColors[3] = LightColor;

	// Tangents
	OutSection.Tangents.SetNum(4);
	OutSection.Tangents[0] = WallTangent;
	OutSection.Tangents[1] = WallTangent;
	OutSection.Tangents[2] = WallTangent;
	OutSection.Tangents[3] = WallTangent;

	// Two triangles forming the quad
	OutSection.Triangles = { 0, 1, 2, 0, 2, 3 };

	OutSection.TextureName = TextureName;
	OutSection.SectorIndex = SectorIdx;
}

// ============================================================================
// GenerateWallSegment - generate wall geometry for a seg
// ============================================================================

void UDoomMeshGenerator::GenerateWallSegment(int32 SegIndex, UDoomLevelLoader* LevelData,
											 TArray<FDoomMeshSection>& OutSections)
{
	OutSections.Empty();

	if (!LevelData || SegIndex < 0 || SegIndex >= LevelData->Segs.Num())
	{
		return;
	}

	const FDoomSeg& Seg = LevelData->Segs[SegIndex];

	// Get vertices
	if (Seg.V1 < 0 || Seg.V1 >= LevelData->Vertexes.Num() ||
		Seg.V2 < 0 || Seg.V2 >= LevelData->Vertexes.Num())
	{
		return;
	}

	const FDoomVertex& Vert1 = LevelData->Vertexes[Seg.V1];
	const FDoomVertex& Vert2 = LevelData->Vertexes[Seg.V2];
	const FVector2D V1(Vert1.X, Vert1.Y);
	const FVector2D V2(Vert2.X, Vert2.Y);

	// Get linedef
	if (Seg.LineDefIndex < 0 || Seg.LineDefIndex >= LevelData->Lines.Num())
	{
		return;
	}

	const FDoomLine& LineDef = LevelData->Lines[Seg.LineDefIndex];

	// Get sidedef
	if (Seg.SideDefIndex < 0 || Seg.SideDefIndex >= LevelData->Sides.Num())
	{
		return;
	}

	const FDoomSide& SideDef = LevelData->Sides[Seg.SideDefIndex];

	// Get front sector
	if (Seg.FrontSector < 0 || Seg.FrontSector >= LevelData->Sectors.Num())
	{
		return;
	}

	const FDoomSector& FrontSector = LevelData->Sectors[Seg.FrontSector];

	const float TexOffset = SideDef.TextureOffset;
	const float RowOffset = SideDef.RowOffset;
	const float SegOffset = Seg.Offset;
	const int32 LightLevel = FrontSector.LightLevel;

	// ----------------------------------------------------------------
	// One-sided linedef: single middle wall from floor to ceiling
	// ----------------------------------------------------------------
	if (Seg.BackSector < 0 || !LineDef.IsTwoSided())
	{
		if (SideDef.MidTexture != 0)
		{
			FDoomMeshSection MiddleWall;
			CreateWallQuad(
				V1, V2,
				FrontSector.FloorHeight,
				FrontSector.CeilingHeight,
				TexOffset, RowOffset, SegOffset,
				LightLevel,
				SideDef.MidTextureName,
				Seg.FrontSector,
				MiddleWall
			);

			if (MiddleWall.IsValid())
			{
				OutSections.Add(MoveTemp(MiddleWall));
			}
		}
		return;
	}

	// ----------------------------------------------------------------
	// Two-sided linedef: upper, lower, and optional middle textures
	// ----------------------------------------------------------------
	if (Seg.BackSector < 0 || Seg.BackSector >= LevelData->Sectors.Num())
	{
		return;
	}

	const FDoomSector& BackSector = LevelData->Sectors[Seg.BackSector];

	// Upper wall: from back sector ceiling to front sector ceiling
	if (FrontSector.CeilingHeight > BackSector.CeilingHeight && SideDef.TopTexture != 0)
	{
		FDoomMeshSection UpperWall;
		CreateWallQuad(
			V1, V2,
			BackSector.CeilingHeight,    // Bottom of upper wall
			FrontSector.CeilingHeight,   // Top of upper wall
			TexOffset, RowOffset, SegOffset,
			LightLevel,
			SideDef.TopTextureName,
			Seg.FrontSector,
			UpperWall
		);

		if (UpperWall.IsValid())
		{
			OutSections.Add(MoveTemp(UpperWall));
		}
	}

	// Lower wall: from front sector floor to back sector floor
	if (FrontSector.FloorHeight < BackSector.FloorHeight && SideDef.BottomTexture != 0)
	{
		FDoomMeshSection LowerWall;
		CreateWallQuad(
			V1, V2,
			FrontSector.FloorHeight,    // Bottom of lower wall
			BackSector.FloorHeight,     // Top of lower wall
			TexOffset, RowOffset, SegOffset,
			LightLevel,
			SideDef.BottomTextureName,
			Seg.FrontSector,
			LowerWall
		);

		if (LowerWall.IsValid())
		{
			OutSections.Add(MoveTemp(LowerWall));
		}
	}

	// Middle texture on two-sided lines (e.g., fences, grates, windows)
	if (SideDef.MidTexture != 0)
	{
		// The middle texture on a two-sided line spans from the higher floor
		// to the lower ceiling (the opening).
		const float OpenBottom = FMath::Max(FrontSector.FloorHeight, BackSector.FloorHeight);
		const float OpenTop = FMath::Min(FrontSector.CeilingHeight, BackSector.CeilingHeight);

		if (OpenTop > OpenBottom)
		{
			FDoomMeshSection MiddleWall;
			CreateWallQuad(
				V1, V2,
				OpenBottom,
				OpenTop,
				TexOffset, RowOffset, SegOffset,
				LightLevel,
				SideDef.MidTextureName,
				Seg.FrontSector,
				MiddleWall
			);

			if (MiddleWall.IsValid())
			{
				OutSections.Add(MoveTemp(MiddleWall));
			}
		}
	}
}
