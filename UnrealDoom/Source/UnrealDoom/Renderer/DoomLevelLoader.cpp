// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// Level loader implementation - mirrors p_setup.c logic.

#include "DoomLevelLoader.h"

// WAD manager forward - the caller provides lump data via UWadManager
// We assume UWadManager has: GetLumpData(int32 LumpNum, TArray<uint8>& OutData)
//                            GetNumForName(const FString& Name) -> int32
//                            GetLumpLength(int32 LumpNum) -> int32
// Adjust the include path to match your project layout:
// #include "WAD/WadManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomLevel, Log, All);

// ============================================================================
// WAD on-disk structures (packed, matching doomdata.h layout)
// These are used only during loading and are NOT USTRUCTs.
// ============================================================================

#pragma pack(push, 1)

struct FWadMapVertex
{
	int16 X;
	int16 Y;
};

struct FWadMapSector
{
	int16 FloorHeight;
	int16 CeilingHeight;
	char FloorPic[8];
	char CeilingPic[8];
	int16 LightLevel;
	int16 Special;
	int16 Tag;
};

struct FWadMapSideDef
{
	int16 TextureOffset;
	int16 RowOffset;
	char TopTexture[8];
	char BottomTexture[8];
	char MidTexture[8];
	int16 Sector;
};

struct FWadMapLineDef
{
	int16 V1;
	int16 V2;
	int16 Flags;
	int16 Special;
	int16 Tag;
	int16 SideNum[2];
};

struct FWadMapSubSector
{
	int16 NumSegs;
	int16 FirstSeg;
};

struct FWadMapNode
{
	int16 X;
	int16 Y;
	int16 Dx;
	int16 Dy;
	int16 BBox[2][4];
	uint16 Children[2];
};

struct FWadMapSeg
{
	int16 V1;
	int16 V2;
	int16 Angle;
	int16 LineDef;
	int16 Side;
	int16 Offset;
};

struct FWadMapThing
{
	int16 X;
	int16 Y;
	int16 Angle;
	int16 Type;
	int16 Options;
};

#pragma pack(pop)

// ML_TWOSIDED flag from doomdata.h
static constexpr int16 ML_TWOSIDED = 4;

// ============================================================================
// Helper functions
// ============================================================================

float UDoomLevelLoader::FixedToUnreal(int32 FixedValue)
{
	// fixed_t is 16.16; convert to float then apply DOOM->Unreal scale
	return static_cast<float>(FixedValue) * FIXED_TO_FLOAT * DOOM_TO_UNREAL_SCALE;
}

float UDoomLevelLoader::MapCoordToUnreal(int16 MapValue)
{
	// Map values are plain integers that get shifted left by FRACBITS in the original.
	// We skip the intermediate fixed_t and go straight to float.
	return static_cast<float>(MapValue) * DOOM_TO_UNREAL_SCALE;
}

FString UDoomLevelLoader::ReadName8(const uint8* Data)
{
	char SafeName[9];
	FMemory::Memcpy(SafeName, Data, 8);
	SafeName[8] = '\0';
	return FString(UTF8_TO_TCHAR(SafeName)).TrimEnd();
}

// ============================================================================
// ClearLevel
// ============================================================================

void UDoomLevelLoader::ClearLevel()
{
	Vertexes.Empty();
	Sectors.Empty();
	Sides.Empty();
	Lines.Empty();
	SubSectors.Empty();
	Nodes.Empty();
	Segs.Empty();
	Things.Empty();
	RejectMatrix.Empty();
	PlayerStarts.Empty();
	DeathmatchStarts.Empty();

	Blockmap = FDoomBlockmap();
	bIsLoaded = false;
}

// ============================================================================
// LoadLevel - main entry point, mirrors P_SetupLevel
// ============================================================================

bool UDoomLevelLoader::LoadLevel(UWadManager* WadManager, int32 Episode, int32 Map)
{
	if (!WadManager)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("LoadLevel: WadManager is null"));
		return false;
	}

	ClearLevel();

	// Build the lump name: E1M1 style or MAP01 style.
	// We assume the WadManager can provide lump data by index.
	// The map header lump is at index N, and sub-lumps are at N+1..N+10.
	// For now, we expect the caller to locate the base lump and pass
	// lump data arrays per sub-lump. This mirrors P_SetupLevel's
	// lumpnum + ML_VERTEXES pattern.
	//
	// Since we do not have UWadManager's actual interface yet,
	// this implementation reads from raw byte arrays that would be
	// provided by the WadManager. The caller should retrieve each
	// ML_* lump and call the individual loaders.

	UE_LOG(LogDoomLevel, Log, TEXT("LoadLevel: Loading E%dM%d"), Episode, Map);

	// NOTE: In a real integration, you would call WadManager methods here:
	//   int32 LumpNum = WadManager->GetNumForName(LumpName);
	//   TArray<uint8> VertexData;
	//   WadManager->GetLumpData(LumpNum + (int32)EMapLump::ML_VERTEXES, VertexData);
	//   P_LoadVertexes(VertexData);
	// ... and so on for each lump.
	//
	// The individual P_Load* methods below are fully functional and expect
	// the raw lump bytes. This stub returns false until the WadManager
	// integration is connected.

	// Placeholder: signal that the individual loaders need WAD data
	UE_LOG(LogDoomLevel, Warning,
		TEXT("LoadLevel: Connect UWadManager to provide lump data for E%dM%d"),
		Episode, Map);

	return false;
}

// ============================================================================
// P_LoadVertexes - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadVertexes(const TArray<uint8>& LumpData)
{
	const int32 NumVertexes = LumpData.Num() / sizeof(FWadMapVertex);
	if (NumVertexes <= 0)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadVertexes: No vertex data"));
		return false;
	}

	Vertexes.SetNum(NumVertexes);
	const FWadMapVertex* MapVerts = reinterpret_cast<const FWadMapVertex*>(LumpData.GetData());

	for (int32 i = 0; i < NumVertexes; i++)
	{
		// Original: li->x = SHORT(ml->x) << FRACBITS;
		// We convert directly to Unreal float units.
		Vertexes[i].X = MapCoordToUnreal(MapVerts[i].X);
		Vertexes[i].Y = MapCoordToUnreal(MapVerts[i].Y);
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadVertexes: Loaded %d vertexes"), NumVertexes);
	return true;
}

// ============================================================================
// P_LoadSectors - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadSectors(const TArray<uint8>& LumpData)
{
	const int32 NumSectors = LumpData.Num() / sizeof(FWadMapSector);
	if (NumSectors <= 0)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadSectors: No sector data"));
		return false;
	}

	Sectors.SetNum(NumSectors);
	const FWadMapSector* MapSectors = reinterpret_cast<const FWadMapSector*>(LumpData.GetData());

	for (int32 i = 0; i < NumSectors; i++)
	{
		FDoomSector& Sector = Sectors[i];

		// Original: ss->floorheight = SHORT(ms->floorheight) << FRACBITS;
		Sector.FloorHeight = MapCoordToUnreal(MapSectors[i].FloorHeight);
		Sector.CeilingHeight = MapCoordToUnreal(MapSectors[i].CeilingHeight);

		// Texture indices would normally come from R_FlatNumForName.
		// Store the names; texture lookup happens later.
		Sector.FloorTextureName = ReadName8(reinterpret_cast<const uint8*>(MapSectors[i].FloorPic));
		Sector.CeilingTextureName = ReadName8(reinterpret_cast<const uint8*>(MapSectors[i].CeilingPic));

		// For now, store a hash-based index as placeholder
		Sector.FloorTexture = GetTypeHash(Sector.FloorTextureName);
		Sector.CeilingTexture = GetTypeHash(Sector.CeilingTextureName);

		Sector.LightLevel = MapSectors[i].LightLevel;
		Sector.Special = MapSectors[i].Special;
		Sector.Tag = MapSectors[i].Tag;

		// Runtime fields initialized to defaults
		Sector.FloorPlane = FPlane(FVector::UpVector, Sector.FloorHeight);
		Sector.CeilingPlane = FPlane(FVector(0.0f, 0.0f, -1.0f), -Sector.CeilingHeight);
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadSectors: Loaded %d sectors"), NumSectors);
	return true;
}

// ============================================================================
// P_LoadSideDefs - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadSideDefs(const TArray<uint8>& LumpData)
{
	const int32 NumSides = LumpData.Num() / sizeof(FWadMapSideDef);
	if (NumSides <= 0)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadSideDefs: No sidedef data"));
		return false;
	}

	Sides.SetNum(NumSides);
	const FWadMapSideDef* MapSides = reinterpret_cast<const FWadMapSideDef*>(LumpData.GetData());

	for (int32 i = 0; i < NumSides; i++)
	{
		FDoomSide& Side = Sides[i];

		// Original: sd->textureoffset = SHORT(msd->textureoffset) << FRACBITS;
		Side.TextureOffset = MapCoordToUnreal(MapSides[i].TextureOffset);
		Side.RowOffset = MapCoordToUnreal(MapSides[i].RowOffset);

		// Texture names - stored for later lookup via R_TextureNumForName equivalent
		Side.TopTextureName = ReadName8(reinterpret_cast<const uint8*>(MapSides[i].TopTexture));
		Side.BottomTextureName = ReadName8(reinterpret_cast<const uint8*>(MapSides[i].BottomTexture));
		Side.MidTextureName = ReadName8(reinterpret_cast<const uint8*>(MapSides[i].MidTexture));

		// Placeholder texture indices
		Side.TopTexture = Side.TopTextureName == TEXT("-") ? 0 : GetTypeHash(Side.TopTextureName);
		Side.BottomTexture = Side.BottomTextureName == TEXT("-") ? 0 : GetTypeHash(Side.BottomTextureName);
		Side.MidTexture = Side.MidTextureName == TEXT("-") ? 0 : GetTypeHash(Side.MidTextureName);

		// Sector reference
		Side.SectorIndex = MapSides[i].Sector;
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadSideDefs: Loaded %d sidedefs"), NumSides);
	return true;
}

// ============================================================================
// P_LoadLineDefs - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadLineDefs(const TArray<uint8>& LumpData)
{
	const int32 NumLines = LumpData.Num() / sizeof(FWadMapLineDef);
	if (NumLines <= 0)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadLineDefs: No linedef data"));
		return false;
	}

	Lines.SetNum(NumLines);
	const FWadMapLineDef* MapLines = reinterpret_cast<const FWadMapLineDef*>(LumpData.GetData());

	for (int32 i = 0; i < NumLines; i++)
	{
		FDoomLine& Line = Lines[i];
		const FWadMapLineDef& ML = MapLines[i];

		Line.Flags = ML.Flags;
		Line.Special = ML.Special;
		Line.Tag = ML.Tag;

		// Vertex indices
		Line.V1 = ML.V1;
		Line.V2 = ML.V2;

		// Precalculated deltas (in Unreal units)
		if (Line.V1 >= 0 && Line.V1 < Vertexes.Num() &&
			Line.V2 >= 0 && Line.V2 < Vertexes.Num())
		{
			const FDoomVertex& Vert1 = Vertexes[Line.V1];
			const FDoomVertex& Vert2 = Vertexes[Line.V2];

			Line.Dx = Vert2.X - Vert1.X;
			Line.Dy = Vert2.Y - Vert1.Y;

			// Slope type
			if (FMath::IsNearlyZero(Line.Dx))
			{
				Line.SlopeType = EDoomSlopeType::Vertical;
			}
			else if (FMath::IsNearlyZero(Line.Dy))
			{
				Line.SlopeType = EDoomSlopeType::Horizontal;
			}
			else if ((Line.Dy / Line.Dx) > 0.0f)
			{
				Line.SlopeType = EDoomSlopeType::Positive;
			}
			else
			{
				Line.SlopeType = EDoomSlopeType::Negative;
			}

			// Bounding box
			Line.BBox[BOXLEFT] = FMath::Min(Vert1.X, Vert2.X);
			Line.BBox[BOXRIGHT] = FMath::Max(Vert1.X, Vert2.X);
			Line.BBox[BOXBOTTOM] = FMath::Min(Vert1.Y, Vert2.Y);
			Line.BBox[BOXTOP] = FMath::Max(Vert1.Y, Vert2.Y);
		}

		// Side numbers
		Line.SideNum[0] = ML.SideNum[0];
		Line.SideNum[1] = ML.SideNum[1];

		// Front/back sector linkage via sidedefs
		if (Line.SideNum[0] >= 0 && Line.SideNum[0] < Sides.Num())
		{
			Line.FrontSector = Sides[Line.SideNum[0]].SectorIndex;
		}
		else
		{
			Line.FrontSector = -1;
		}

		if (Line.SideNum[1] >= 0 && Line.SideNum[1] < Sides.Num())
		{
			Line.BackSector = Sides[Line.SideNum[1]].SectorIndex;
		}
		else
		{
			Line.BackSector = -1;
		}
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadLineDefs: Loaded %d linedefs"), NumLines);
	return true;
}

// ============================================================================
// P_LoadSubsectors - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadSubsectors(const TArray<uint8>& LumpData)
{
	const int32 NumSubSectors = LumpData.Num() / sizeof(FWadMapSubSector);
	if (NumSubSectors <= 0)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadSubsectors: No subsector data"));
		return false;
	}

	SubSectors.SetNum(NumSubSectors);
	const FWadMapSubSector* MapSS = reinterpret_cast<const FWadMapSubSector*>(LumpData.GetData());

	for (int32 i = 0; i < NumSubSectors; i++)
	{
		SubSectors[i].NumLines = MapSS[i].NumSegs;
		SubSectors[i].FirstLine = MapSS[i].FirstSeg;
		// SectorIndex will be set in P_GroupLines
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadSubsectors: Loaded %d subsectors"), NumSubSectors);
	return true;
}

// ============================================================================
// P_LoadNodes - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadNodes(const TArray<uint8>& LumpData)
{
	const int32 NumNodeCount = LumpData.Num() / sizeof(FWadMapNode);
	if (NumNodeCount <= 0)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadNodes: No node data"));
		return false;
	}

	Nodes.SetNum(NumNodeCount);
	const FWadMapNode* MapNodes = reinterpret_cast<const FWadMapNode*>(LumpData.GetData());

	for (int32 i = 0; i < NumNodeCount; i++)
	{
		FDoomNode& Node = Nodes[i];
		const FWadMapNode& MN = MapNodes[i];

		// Partition line (original shifts by FRACBITS, we go straight to float)
		Node.X = MapCoordToUnreal(MN.X);
		Node.Y = MapCoordToUnreal(MN.Y);
		Node.Dx = MapCoordToUnreal(MN.Dx);
		Node.Dy = MapCoordToUnreal(MN.Dy);

		// Children (keep raw uint16 - NF_SUBSECTOR flag preserved)
		Node.Children[0] = MN.Children[0];
		Node.Children[1] = MN.Children[1];

		// Bounding boxes
		for (int32 j = 0; j < 2; j++)
		{
			for (int32 k = 0; k < 4; k++)
			{
				Node.BBox[j][k] = MapCoordToUnreal(MN.BBox[j][k]);
			}
		}
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadNodes: Loaded %d nodes"), NumNodeCount);
	return true;
}

// ============================================================================
// P_LoadSegs - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadSegs(const TArray<uint8>& LumpData)
{
	const int32 NumSegsCount = LumpData.Num() / sizeof(FWadMapSeg);
	if (NumSegsCount <= 0)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadSegs: No seg data"));
		return false;
	}

	Segs.SetNum(NumSegsCount);
	const FWadMapSeg* MapSegs = reinterpret_cast<const FWadMapSeg*>(LumpData.GetData());

	for (int32 i = 0; i < NumSegsCount; i++)
	{
		FDoomSeg& Seg = Segs[i];
		const FWadMapSeg& MS = MapSegs[i];

		Seg.V1 = MS.V1;
		Seg.V2 = MS.V2;

		// Angle: original does (SHORT(ml->angle)) << 16
		Seg.Angle = static_cast<int64>(MS.Angle) << 16;

		// Offset along linedef: original does (SHORT(ml->offset)) << 16, then used as fixed_t
		// Convert to Unreal units
		Seg.Offset = MapCoordToUnreal(MS.Offset);

		// Linedef reference
		const int32 LineDefIdx = MS.LineDef;
		Seg.LineDefIndex = LineDefIdx;

		if (LineDefIdx >= 0 && LineDefIdx < Lines.Num())
		{
			const FDoomLine& LineDef = Lines[LineDefIdx];
			const int32 Side = MS.Side;

			// Sidedef
			if (Side >= 0 && Side <= 1 && LineDef.SideNum[Side] >= 0)
			{
				Seg.SideDefIndex = LineDef.SideNum[Side];

				// Front sector comes from the sidedef's sector
				if (Seg.SideDefIndex >= 0 && Seg.SideDefIndex < Sides.Num())
				{
					Seg.FrontSector = Sides[Seg.SideDefIndex].SectorIndex;
				}
			}

			// Back sector: if two-sided, use the opposite side's sector
			if (LineDef.Flags & ML_TWOSIDED)
			{
				const int32 OtherSide = Side ^ 1;
				if (OtherSide >= 0 && OtherSide <= 1 && LineDef.SideNum[OtherSide] >= 0)
				{
					const int32 OtherSideIdx = LineDef.SideNum[OtherSide];
					if (OtherSideIdx >= 0 && OtherSideIdx < Sides.Num())
					{
						Seg.BackSector = Sides[OtherSideIdx].SectorIndex;
					}
				}
			}
			else
			{
				Seg.BackSector = -1;
			}
		}
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadSegs: Loaded %d segs"), NumSegsCount);
	return true;
}

// ============================================================================
// P_LoadBlockMap - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadBlockMap(const TArray<uint8>& LumpData)
{
	const int32 Count = LumpData.Num() / sizeof(int16);
	if (Count < 4)
	{
		UE_LOG(LogDoomLevel, Error, TEXT("P_LoadBlockMap: Blockmap lump too small"));
		return false;
	}

	const int16* RawData = reinterpret_cast<const int16*>(LumpData.GetData());

	// First 4 entries are the header
	Blockmap.OriginX = MapCoordToUnreal(RawData[0]);
	Blockmap.OriginY = MapCoordToUnreal(RawData[1]);
	Blockmap.Width = RawData[2];
	Blockmap.Height = RawData[3];

	// Store the full lump
	Blockmap.BlockmapLump.SetNum(Count);
	for (int32 i = 0; i < Count; i++)
	{
		Blockmap.BlockmapLump[i] = RawData[i];
	}

	// Offsets start at index 4 (blockmap = blockmaplump + 4)
	const int32 NumOffsets = Blockmap.Width * Blockmap.Height;
	if (Count >= 4 + NumOffsets)
	{
		Blockmap.Offsets.SetNum(NumOffsets);
		for (int32 i = 0; i < NumOffsets; i++)
		{
			Blockmap.Offsets[i] = RawData[4 + i];
		}
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadBlockMap: %dx%d blocks"), Blockmap.Width, Blockmap.Height);
	return true;
}

// ============================================================================
// P_LoadThings - from p_setup.c
// ============================================================================

bool UDoomLevelLoader::P_LoadThings(const TArray<uint8>& LumpData)
{
	const int32 NumThings = LumpData.Num() / sizeof(FWadMapThing);
	if (NumThings <= 0)
	{
		UE_LOG(LogDoomLevel, Warning, TEXT("P_LoadThings: No things in lump"));
		return true; // Not an error - some maps might have no things in theory
	}

	const FWadMapThing* MapThings = reinterpret_cast<const FWadMapThing*>(LumpData.GetData());

	Things.Reserve(NumThings);

	for (int32 i = 0; i < NumThings; i++)
	{
		FDoomMapThing Thing;
		Thing.X = MapCoordToUnreal(MapThings[i].X);
		Thing.Y = MapCoordToUnreal(MapThings[i].Y);
		Thing.Angle = MapThings[i].Angle;
		Thing.Type = MapThings[i].Type;
		Thing.Options = MapThings[i].Options;

		// Player starts (types 1-4) go into the PlayerStarts array
		if (Thing.Type >= 1 && Thing.Type <= 4)
		{
			PlayerStarts.Add(Thing);
		}
		// Deathmatch starts (type 11)
		else if (Thing.Type == 11)
		{
			DeathmatchStarts.Add(Thing);
		}

		Things.Add(Thing);
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_LoadThings: Loaded %d things (%d player starts, %d DM starts)"),
		Things.Num(), PlayerStarts.Num(), DeathmatchStarts.Num());
	return true;
}

// ============================================================================
// P_GroupLines - from p_setup.c
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
// ============================================================================

void UDoomLevelLoader::P_GroupLines()
{
	// Step 1: Look up sector number for each subsector
	for (int32 i = 0; i < SubSectors.Num(); i++)
	{
		FDoomSubSector& SS = SubSectors[i];
		if (SS.FirstLine >= 0 && SS.FirstLine < Segs.Num())
		{
			const FDoomSeg& FirstSeg = Segs[SS.FirstLine];
			if (FirstSeg.SideDefIndex >= 0 && FirstSeg.SideDefIndex < Sides.Num())
			{
				SS.SectorIndex = Sides[FirstSeg.SideDefIndex].SectorIndex;
			}
		}
	}

	// Step 2: Count number of lines in each sector
	for (FDoomSector& Sector : Sectors)
	{
		Sector.LineCount = 0;
		Sector.Lines.Empty();
	}

	for (int32 i = 0; i < Lines.Num(); i++)
	{
		const FDoomLine& Line = Lines[i];

		if (Line.FrontSector >= 0 && Line.FrontSector < Sectors.Num())
		{
			Sectors[Line.FrontSector].LineCount++;
			Sectors[Line.FrontSector].Lines.Add(i);
		}

		if (Line.BackSector >= 0 && Line.BackSector < Sectors.Num() &&
			Line.BackSector != Line.FrontSector)
		{
			Sectors[Line.BackSector].LineCount++;
			Sectors[Line.BackSector].Lines.Add(i);
		}
	}

	// Step 3: Build bounding boxes and sound origins for each sector
	for (int32 i = 0; i < Sectors.Num(); i++)
	{
		FDoomSector& Sector = Sectors[i];

		float MinX = MAX_FLT;
		float MaxX = -MAX_FLT;
		float MinY = MAX_FLT;
		float MaxY = -MAX_FLT;

		for (int32 LineIdx : Sector.Lines)
		{
			if (LineIdx < 0 || LineIdx >= Lines.Num())
			{
				continue;
			}

			const FDoomLine& Line = Lines[LineIdx];

			if (Line.V1 >= 0 && Line.V1 < Vertexes.Num())
			{
				const FDoomVertex& V = Vertexes[Line.V1];
				MinX = FMath::Min(MinX, V.X);
				MaxX = FMath::Max(MaxX, V.X);
				MinY = FMath::Min(MinY, V.Y);
				MaxY = FMath::Max(MaxY, V.Y);
			}
			if (Line.V2 >= 0 && Line.V2 < Vertexes.Num())
			{
				const FDoomVertex& V = Vertexes[Line.V2];
				MinX = FMath::Min(MinX, V.X);
				MaxX = FMath::Max(MaxX, V.X);
				MinY = FMath::Min(MinY, V.Y);
				MaxY = FMath::Max(MaxY, V.Y);
			}
		}

		// Sound origin at center of bounding box
		if (MinX <= MaxX && MinY <= MaxY)
		{
			Sector.SoundOrigin = FVector(
				(MinX + MaxX) * 0.5f,
				(MinY + MaxY) * 0.5f,
				(Sector.FloorHeight + Sector.CeilingHeight) * 0.5f
			);
		}

		// Update planes
		Sector.FloorPlane = FPlane(FVector::UpVector, Sector.FloorHeight);
		Sector.CeilingPlane = FPlane(FVector(0.0f, 0.0f, -1.0f), -Sector.CeilingHeight);
	}

	UE_LOG(LogDoomLevel, Log, TEXT("P_GroupLines: Grouped lines for %d sectors"), Sectors.Num());
}
