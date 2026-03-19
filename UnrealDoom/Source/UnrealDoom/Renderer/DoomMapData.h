#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// Runtime map geometry structures - the "loaded" representation of DOOM's
// level data. These mirror the original vertex_t, sector_t, side_t, line_t,
// seg_t, subsector_t, node_t from r_defs.h, converted to UE5 conventions
// with floating-point coordinates.

#include "CoreMinimal.h"
#include "DoomMapData.generated.h"

// Scale factor: 1 DOOM unit = 2.0 Unreal units
constexpr float DOOM_TO_UNREAL_SCALE = 2.0f;

// Fixed-point conversion: DOOM uses 16.16 fixed-point
constexpr float FIXED_TO_FLOAT = 1.0f / 65536.0f;

// NF_SUBSECTOR flag - leaf node indicator in BSP tree
constexpr uint16 NF_SUBSECTOR = 0x8000;

// Bounding box indices (from m_bbox.h)
constexpr int32 BOXTOP = 0;
constexpr int32 BOXBOTTOM = 1;
constexpr int32 BOXLEFT = 2;
constexpr int32 BOXRIGHT = 3;

// ============================================================================
// Slope type for linedefs (from r_defs.h slopetype_t)
// ============================================================================

UENUM(BlueprintType)
enum class EDoomSlopeType : uint8
{
	Horizontal = 0,   // ST_HORIZONTAL
	Vertical = 1,     // ST_VERTICAL
	Positive = 2,     // ST_POSITIVE
	Negative = 3      // ST_NEGATIVE
};

// ============================================================================
// FDoomVertex - Runtime vertex (from vertex_t)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomVertex
{
	GENERATED_BODY()

	/** X position in Unreal world units (converted from fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float X = 0.0f;

	/** Y position in Unreal world units (converted from fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Y = 0.0f;

	/** Convert to UE FVector2D */
	FVector2D ToVector2D() const { return FVector2D(X, Y); }

	/** Convert to UE FVector (X maps to UE X, Y maps to UE Y, Z=0) */
	FVector ToVector(float Z = 0.0f) const { return FVector(X, Y, Z); }
};

// ============================================================================
// FDoomSector - Runtime sector (from sector_t)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomSector
{
	GENERATED_BODY()

	/** Floor height in Unreal units (converted from fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float FloorHeight = 0.0f;

	/** Ceiling height in Unreal units (converted from fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float CeilingHeight = 0.0f;

	/** Floor texture index (flat number) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 FloorTexture = 0;

	/** Ceiling texture index (flat number) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 CeilingTexture = 0;

	/** Light level 0-255 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 LightLevel = 0;

	/** Sector special type (damage, lighting effects, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Special = 0;

	/** Sector tag for trigger identification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Tag = 0;

	// --- Runtime fields (populated during P_GroupLines equivalent) ---

	/** Floor plane for rendering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FPlane FloorPlane = FPlane(FVector::UpVector, 0.0f);

	/** Ceiling plane for rendering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FPlane CeilingPlane = FPlane(FVector::UpVector, 0.0f);

	/** Number of lines bounding this sector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 LineCount = 0;

	/** Indices into the level's Lines array for all lines touching this sector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	TArray<int32> Lines;

	/** Sound traversal marker (0 = untraversed, 1,2 = sndlines -1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SoundTraversed = 0;

	/** Index of the thing that made a sound (-1 = none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SoundTarget = -1;

	/** Mapblock bounding box for height changes [BOXTOP, BOXBOTTOM, BOXLEFT, BOXRIGHT] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 BlockBox[4] = { 0, 0, 0, 0 };

	/** Origin for any sounds played by the sector (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FVector SoundOrigin = FVector::ZeroVector;

	/** Validcount for traversal deduplication */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Validcount = 0;

	/** Pointer-equivalent: index into a special data array, or -1 for none.
	 *  Used for reversible thinker actions (doors, lifts, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SpecialData = -1;

	/** Indices of things currently in this sector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	TArray<int32> ThingList;

	/** Floor texture name (for lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FString FloorTextureName;

	/** Ceiling texture name (for lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FString CeilingTextureName;
};

// ============================================================================
// FDoomSide - Runtime sidedef (from side_t)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomSide
{
	GENERATED_BODY()

	/** Horizontal texture offset in Unreal units (converted from fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float TextureOffset = 0.0f;

	/** Vertical texture offset in Unreal units (converted from fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float RowOffset = 0.0f;

	/** Upper texture index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 TopTexture = 0;

	/** Lower texture index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 BottomTexture = 0;

	/** Middle texture index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 MidTexture = 0;

	/** Index of the sector this sidedef faces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SectorIndex = -1;

	/** Texture names for lookup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FString TopTextureName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FString BottomTextureName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	FString MidTextureName;
};

// ============================================================================
// FDoomLine - Runtime linedef (from line_t)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomLine
{
	GENERATED_BODY()

	/** Index of vertex 1 in the Vertexes array */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 V1 = 0;

	/** Index of vertex 2 in the Vertexes array */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 V2 = 0;

	/** Precalculated delta: V2.x - V1.x (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Dx = 0.0f;

	/** Precalculated delta: V2.y - V1.y (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Dy = 0.0f;

	/** Line attribute flags (ML_BLOCKING, ML_TWOSIDED, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Flags = 0;

	/** Line special type (door, lift, teleporter, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Special = 0;

	/** Sector tag for trigger matching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Tag = 0;

	/** Side indices. SideNum[1] == -1 if one-sided */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SideNum[2] = { -1, -1 };

	/** Bounding box [BOXTOP, BOXBOTTOM, BOXLEFT, BOXRIGHT] in Unreal units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float BBox[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	/** Slope type for move clipping optimization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	EDoomSlopeType SlopeType = EDoomSlopeType::Horizontal;

	/** Front sector index (-1 if none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 FrontSector = -1;

	/** Back sector index (-1 if one-sided) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 BackSector = -1;

	/** Validcount for traversal deduplication */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Validcount = 0;

	/** Index into special data array, or -1 for none */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SpecialData = -1;

	/** Is this a two-sided line? */
	bool IsTwoSided() const { return SideNum[1] != -1; }
};

// ============================================================================
// FDoomSeg - Runtime line segment (from seg_t)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomSeg
{
	GENERATED_BODY()

	/** Index of vertex 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 V1 = 0;

	/** Index of vertex 2 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 V2 = 0;

	/** Offset distance along linedef in Unreal units (converted from fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Offset = 0.0f;

	/** Binary angle (angle_t, kept as uint32 for precision) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int64 Angle = 0;

	/** Index of the sidedef this seg uses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SideDefIndex = -1;

	/** Index of the linedef this seg was split from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 LineDefIndex = -1;

	/** Front sector index */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 FrontSector = -1;

	/** Back sector index (-1 for one-sided lines) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 BackSector = -1;
};

// ============================================================================
// FDoomSubSector - Runtime subsector / BSP leaf (from subsector_t)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomSubSector
{
	GENERATED_BODY()

	/** Number of segs in this subsector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 NumLines = 0;

	/** Index of the first seg in the Segs array */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 FirstLine = 0;

	/** Index of the sector this subsector belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SectorIndex = -1;
};

// ============================================================================
// FDoomNode - BSP node (from node_t)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomNode
{
	GENERATED_BODY()

	/** Partition line origin X (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float X = 0.0f;

	/** Partition line origin Y (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Y = 0.0f;

	/** Partition line direction X (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Dx = 0.0f;

	/** Partition line direction Y (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Dy = 0.0f;

	/**
	 * Bounding box for each child: [0]=right, [1]=left
	 * Each box: [BOXTOP, BOXBOTTOM, BOXLEFT, BOXRIGHT] in Unreal units
	 */
	float BBox[2][4] = {};

	/**
	 * Child references. If NF_SUBSECTOR (0x8000) bit is set,
	 * the lower bits index into the SubSectors array.
	 * Otherwise it indexes into the Nodes array.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	uint16 Children[2] = { 0, 0 };

	/** Check if a child index refers to a subsector */
	static bool IsSubSector(uint16 Child) { return (Child & NF_SUBSECTOR) != 0; }

	/** Extract subsector index from a child reference */
	static int32 GetSubSectorIndex(uint16 Child) { return static_cast<int32>(Child & ~NF_SUBSECTOR); }
};

// ============================================================================
// FDoomBlockmap - Blockmap data for spatial collision queries
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomBlockmap
{
	GENERATED_BODY()

	/** Origin X of the blockmap grid (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float OriginX = 0.0f;

	/** Origin Y of the blockmap grid (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float OriginY = 0.0f;

	/** Width of the blockmap in blocks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Width = 0;

	/** Height of the blockmap in blocks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Height = 0;

	/** Raw blockmap lump data (offsets and line lists) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	TArray<int16> BlockmapLump;

	/** Offset table: index = block index, value = offset into BlockmapLump.
	 *  Starts at blockmaplump + 4 in the original. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	TArray<int16> Offsets;
};

// ============================================================================
// FDoomMapThing - Thing spawn data (converted coordinates)
// ============================================================================

USTRUCT(BlueprintType)
struct FDoomMapThing
{
	GENERATED_BODY()

	/** Position X in Unreal units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float X = 0.0f;

	/** Position Y in Unreal units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	float Y = 0.0f;

	/** Facing angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Angle = 0;

	/** DoomEd thing type number */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Type = 0;

	/** Spawn flags (skill bits, ambush, multiplayer) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 Options = 0;
};
