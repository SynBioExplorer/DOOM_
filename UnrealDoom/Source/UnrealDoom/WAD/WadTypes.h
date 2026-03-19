#pragma once

#include "CoreMinimal.h"
#include "WadTypes.generated.h"

// ============================================================================
// WAD File Format Structures
// Ported from linuxdoom-1.10 w_wad.h / doomdata.h
// ============================================================================

/** WAD type identification */
UENUM(BlueprintType)
enum class EWadType : uint8
{
	IWAD = 0,	// Internal WAD (official game data)
	PWAD = 1,	// Patch WAD (mod/addon data)
	Invalid = 255
};

/** Game mode detection based on loaded WADs */
UENUM(BlueprintType)
enum class EDoomGameMode : uint8
{
	Shareware = 0,		// DOOM1 shareware (E1 only)
	Registered = 1,		// DOOM1 registered (E1-E3)
	Commercial = 2,		// DOOM2 commercial
	Retail = 3,			// Ultimate DOOM (E1-E4)
	Indetermined = 255
};

/** Map lump ordering within a WAD - matches original ML_* enum */
UENUM(BlueprintType)
enum class EMapLump : uint8
{
	ML_LABEL = 0,		// A separator, name, ExMx or MAPxx
	ML_THINGS,			// Monsters, items..
	ML_LINEDEFS,		// LineDefs, from editing
	ML_SIDEDEFS,		// SideDefs, from editing
	ML_VERTEXES,		// Vertices, edited and BSP splits generated
	ML_SEGS,			// LineSegs, from LineDefs split by BSP
	ML_SSECTORS,		// SubSectors, list of LineSegs
	ML_NODES,			// BSP nodes
	ML_SECTORS,			// Sectors, from editing
	ML_REJECT,			// LUT, sector-sector visibility
	ML_BLOCKMAP			// LUT, motion clipping, walls/grid element
};

// ============================================================================
// WAD Header & Directory Structures (from wadinfo_t / filelump_t / lumpinfo_t)
// ============================================================================

/**
 * WAD file header - matches wadinfo_t from w_wad.h
 * Should be "IWAD" or "PWAD" identification.
 */
USTRUCT(BlueprintType)
struct FWadHeader
{
	GENERATED_BODY()

	/** "IWAD" or "PWAD" - 4 character identification */
	char Identification[4];

	/** Number of lumps in the WAD */
	UPROPERTY(BlueprintReadOnly, Category = "WAD")
	int32 NumLumps = 0;

	/** File offset to the info table (directory) */
	UPROPERTY(BlueprintReadOnly, Category = "WAD")
	int32 InfoTableOfs = 0;
};

/**
 * WAD lump info entry - runtime info for each lump.
 * Matches lumpinfo_t from w_wad.h, adapted for UE5.
 */
USTRUCT(BlueprintType)
struct FWadLumpInfo
{
	GENERATED_BODY()

	/** File position (byte offset) of the lump data */
	UPROPERTY(BlueprintReadOnly, Category = "WAD")
	int32 FilePos = 0;

	/** Size of the lump in bytes */
	UPROPERTY(BlueprintReadOnly, Category = "WAD")
	int32 Size = 0;

	/** Lump name, up to 8 characters, uppercase */
	char Name[8] = {};

	/** Index of the WAD file this lump belongs to */
	UPROPERTY(BlueprintReadOnly, Category = "WAD")
	int32 WadFileIndex = 0;

	/** Get name as FString */
	FString GetName() const
	{
		// Name may not be null-terminated if all 8 chars are used
		char SafeName[9];
		FMemory::Memcpy(SafeName, Name, 8);
		SafeName[8] = '\0';
		return FString(UTF8_TO_TCHAR(SafeName)).TrimEnd();
	}

	/** Set name from FString (converts to uppercase, max 8 chars) */
	void SetName(const FString& InName)
	{
		FMemory::Memzero(Name, 8);
		FString Upper = InName.ToUpper().Left(8);
		const auto Converted = StringCast<ANSICHAR>(*Upper);
		FMemory::Memcpy(Name, Converted.Get(), FMath::Min(Upper.Len(), 8));
	}
};

// ============================================================================
// Map Data Structures (from doomdata.h)
// ============================================================================

/**
 * Thing definition - position, orientation, type, plus skill/visibility flags.
 * Matches mapthing_t from doomdata.h
 */
USTRUCT(BlueprintType)
struct FMapThing
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 X = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Y = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Angle = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Type = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Options = 0;
};

/**
 * LineDef - defines a line from vertex to vertex, with flags and sidedefs.
 * Matches maplinedef_t from doomdata.h
 * sidenum[1] will be -1 if one sided.
 */
USTRUCT(BlueprintType)
struct FMapLineDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 V1 = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 V2 = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Flags = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Special = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Tag = 0;

	/** Side numbers. sidenum[1] == -1 if one-sided */
	int16 SideNum[2] = { 0, -1 };
};

/**
 * SideDef - visual appearance of a wall segment.
 * Matches mapsidedef_t from doomdata.h
 */
USTRUCT(BlueprintType)
struct FMapSideDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 TextureOffset = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 RowOffset = 0;

	/** Upper texture name (8 chars max) */
	char TopTexture[8] = {};

	/** Lower texture name (8 chars max) */
	char BottomTexture[8] = {};

	/** Middle texture name (8 chars max) */
	char MidTexture[8] = {};

	/** Front sector number */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Sector = 0;
};

/**
 * Vertex - a single map vertex.
 * Matches mapvertex_t from doomdata.h
 */
USTRUCT(BlueprintType)
struct FMapVertex
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 X = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Y = 0;
};

/**
 * LineSeg - generated by splitting LineDefs using BSP partition lines.
 * Matches mapseg_t from doomdata.h
 */
USTRUCT(BlueprintType)
struct FMapSeg
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 V1 = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 V2 = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Angle = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 LineDef = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Side = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Offset = 0;
};

/**
 * SubSector - as generated by BSP builder.
 * Matches mapsubsector_t from doomdata.h
 */
USTRUCT(BlueprintType)
struct FMapSubSector
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 NumSegs = 0;

	/** Index of first seg; segs are stored sequentially */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 FirstSeg = 0;
};

/**
 * BSP node structure.
 * Matches mapnode_t from doomdata.h
 * NF_SUBSECTOR (0x8000) indicates a leaf node pointing to a subsector.
 */
USTRUCT(BlueprintType)
struct FMapNode
{
	GENERATED_BODY()

	/** Partition line origin */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 X = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Y = 0;

	/** Partition line direction */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 DX = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 DY = 0;

	/**
	 * Bounding box for each child: [0]=right, [1]=left
	 * Each box has [top, bottom, left, right]
	 */
	int16 BBox[2][4] = {};

	/**
	 * If NF_SUBSECTOR (0x8000) bit is set, it's a subsector index,
	 * otherwise it's a child node index.
	 */
	uint16 Children[2] = {};

	/** NF_SUBSECTOR flag - indicates a leaf pointing to a subsector */
	static constexpr uint16 NF_SUBSECTOR = 0x8000;
};

/**
 * Sector definition from editing.
 * Matches mapsector_t from doomdata.h
 */
USTRUCT(BlueprintType)
struct FMapSector
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 FloorHeight = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 CeilingHeight = 0;

	/** Floor flat texture name (8 chars max) */
	char FloorPic[8] = {};

	/** Ceiling flat texture name (8 chars max) */
	char CeilingPic[8] = {};

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 LightLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Special = 0;

	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 Tag = 0;
};

/**
 * Blockmap header - precedes the blockmap data in the BLOCKMAP lump.
 * The blockmap is a LUT used for motion clipping against walls.
 */
USTRUCT(BlueprintType)
struct FMapBlockmapHeader
{
	GENERATED_BODY()

	/** Grid origin X (in map units) */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 OriginX = 0;

	/** Grid origin Y (in map units) */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 OriginY = 0;

	/** Number of columns (blocks in X direction) */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 NumColumns = 0;

	/** Number of rows (blocks in Y direction) */
	UPROPERTY(BlueprintReadOnly, Category = "WAD|Map")
	int16 NumRows = 0;
};

// ============================================================================
// LineDef attribute flags - matches ML_* defines from doomdata.h
// ============================================================================

namespace DoomLineFlags
{
	constexpr int16 ML_BLOCKING        = 1;    // Solid, is an obstacle
	constexpr int16 ML_BLOCKMONSTERS   = 2;    // Blocks monsters only
	constexpr int16 ML_TWOSIDED        = 4;    // Backside will not be present if not two sided
	constexpr int16 ML_DONTPEGTOP      = 8;    // Upper texture unpegged
	constexpr int16 ML_DONTPEGBOTTOM   = 16;   // Lower texture unpegged
	constexpr int16 ML_SECRET          = 32;   // In AutoMap: don't map as two sided: IT'S A SECRET!
	constexpr int16 ML_SOUNDBLOCK      = 64;   // Sound rendering: don't let sound cross two of these
	constexpr int16 ML_DONTDRAW        = 128;  // Don't draw on the automap at all
	constexpr int16 ML_MAPPED          = 256;  // Set if already seen, thus drawn in automap
}
