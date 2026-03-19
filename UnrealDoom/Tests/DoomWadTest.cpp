// DoomWadTest.cpp - Tests for WAD file loading and lump management
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "WAD/WadTypes.h"
#include "WAD/WadFile.h"
#include "WAD/WadManager.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// WAD Header Validation Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomWadHeaderTest, "UnrealDoom.WAD.HeaderValidation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomWadHeaderTest::RunTest(const FString& Parameters)
{
    // Test WAD identification strings
    FWadHeader IwadHeader;
    FMemory::Memcpy(IwadHeader.Identification, "IWAD", 4);
    IwadHeader.NumLumps = 0;
    IwadHeader.InfoTableOfs = 12;

    TestTrue(TEXT("IWAD identified"),
        IwadHeader.Identification[0] == 'I' &&
        IwadHeader.Identification[1] == 'W' &&
        IwadHeader.Identification[2] == 'A' &&
        IwadHeader.Identification[3] == 'D');

    FWadHeader PwadHeader;
    FMemory::Memcpy(PwadHeader.Identification, "PWAD", 4);
    TestTrue(TEXT("PWAD identified"),
        PwadHeader.Identification[0] == 'P' &&
        PwadHeader.Identification[1] == 'W' &&
        PwadHeader.Identification[2] == 'A' &&
        PwadHeader.Identification[3] == 'D');

    // Invalid header
    FWadHeader BadHeader;
    FMemory::Memcpy(BadHeader.Identification, "NOPE", 4);
    TestTrue(TEXT("Invalid header rejected"),
        !(BadHeader.Identification[0] == 'I' || BadHeader.Identification[0] == 'P') ||
        BadHeader.Identification[1] != 'W' ||
        BadHeader.Identification[2] != 'A' ||
        BadHeader.Identification[3] != 'D');

    return true;
}

// ============================================================================
// Lump Name Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomWadLumpNameTest, "UnrealDoom.WAD.LumpNames",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomWadLumpNameTest::RunTest(const FString& Parameters)
{
    // Lump names are 8 chars, null-padded
    FWadLumpInfo Lump;
    FMemory::Memzero(&Lump, sizeof(FWadLumpInfo));

    // Set a standard lump name
    FMemory::Memcpy(Lump.Name, "E1M1\0\0\0\0", 8);
    FString Name = FString(8, Lump.Name).TrimEnd();
    TestEqual(TEXT("E1M1 lump name"), Name, TEXT("E1M1"));

    // Full 8-character name
    FMemory::Memcpy(Lump.Name, "PLAYPAL\0", 8);
    FString FullName = FString(8, Lump.Name).TrimEnd();
    TestEqual(TEXT("PLAYPAL lump name"), FullName, TEXT("PLAYPAL"));

    // 8-char name with no null terminator
    FMemory::Memcpy(Lump.Name, "TEXTURES", 8);
    FString MaxName(8, Lump.Name);
    TestEqual(TEXT("8-char name"), MaxName.Len(), 8);

    // Standard map lumps that must exist
    TArray<FString> RequiredLumps = {
        TEXT("THINGS"), TEXT("LINEDEFS"), TEXT("SIDEDEFS"), TEXT("VERTEXES"),
        TEXT("SEGS"), TEXT("SSECTORS"), TEXT("NODES"), TEXT("SECTORS"),
        TEXT("REJECT"), TEXT("BLOCKMAP")
    };

    for (const FString& LumpName : RequiredLumps)
    {
        TestTrue(*FString::Printf(TEXT("Map lump %s <= 8 chars"), *LumpName),
            LumpName.Len() <= 8);
    }

    return true;
}

// ============================================================================
// Map Data Structure Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomMapStructTest, "UnrealDoom.WAD.MapStructures",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomMapStructTest::RunTest(const FString& Parameters)
{
    // Test FMapThing (mapthing_t)
    FMapThing Thing;
    Thing.X = 1056;
    Thing.Y = -3616;
    Thing.Angle = 90;
    Thing.Type = 1; // Player 1 start
    Thing.Options = 7; // All skill levels
    TestEqual(TEXT("Thing X"), Thing.X, (int16)1056);
    TestEqual(TEXT("Thing Y"), Thing.Y, (int16)-3616);
    TestEqual(TEXT("Thing angle"), Thing.Angle, (int16)90);
    TestEqual(TEXT("Player 1 start type"), Thing.Type, (int16)1);

    // Thing options flags
    TestTrue(TEXT("Easy skill"), (Thing.Options & 1) != 0);
    TestTrue(TEXT("Normal skill"), (Thing.Options & 2) != 0);
    TestTrue(TEXT("Hard skill"), (Thing.Options & 4) != 0);
    TestFalse(TEXT("Not deaf"), (Thing.Options & 8) != 0);

    // Test FMapLineDef
    FMapLineDef Line;
    Line.V1 = 0;
    Line.V2 = 1;
    Line.Flags = 1;  // ML_BLOCKING
    Line.Special = 0;
    Line.Tag = 0;
    Line.SideNum[0] = 0;
    Line.SideNum[1] = 0xFFFF; // One-sided line (-1)
    TestEqual(TEXT("Line V1"), Line.V1, (int16)0);
    TestTrue(TEXT("Line is blocking"), (Line.Flags & 1) != 0);
    TestEqual(TEXT("No back side"), (uint16)Line.SideNum[1], (uint16)0xFFFF);

    // Test FMapSector
    FMapSector Sector;
    Sector.FloorHeight = 0;
    Sector.CeilingHeight = 128;
    FMemory::Memcpy(Sector.FloorPic, "FLOOR4_8", 8);
    FMemory::Memcpy(Sector.CeilingPic, "CEIL3_5\0", 8);
    Sector.LightLevel = 160;
    Sector.Special = 0;
    Sector.Tag = 0;
    TestEqual(TEXT("Floor height"), Sector.FloorHeight, (int16)0);
    TestEqual(TEXT("Ceiling height"), Sector.CeilingHeight, (int16)128);
    TestEqual(TEXT("Light level"), Sector.LightLevel, (int16)160);

    return true;
}

// ============================================================================
// WAD File Object Tests (mock/unit)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomWadFileObjectTest, "UnrealDoom.WAD.FileObject",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomWadFileObjectTest::RunTest(const FString& Parameters)
{
    // Test WadFile creation
    UWadFile* WadFile = NewObject<UWadFile>();
    TestNotNull(TEXT("WadFile created"), WadFile);

    // Opening a non-existent file should fail gracefully
    bool bResult = WadFile->OpenWadFile(TEXT("/nonexistent/doom.wad"));
    TestFalse(TEXT("Non-existent WAD fails"), bResult);

    // Initial state should be empty
    TestEqual(TEXT("Empty WAD has 0 lumps"), WadFile->GetNumLumps(), 0);

    // Finding a lump in empty WAD returns -1
    int32 LumpIdx = WadFile->FindLump(TEXT("E1M1"));
    TestEqual(TEXT("No lumps found"), LumpIdx, -1);

    return true;
}

// ============================================================================
// WAD Manager Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomWadManagerTest, "UnrealDoom.WAD.Manager",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomWadManagerTest::RunTest(const FString& Parameters)
{
    // Test WadManager creation
    UWadManager* Manager = NewObject<UWadManager>();
    TestNotNull(TEXT("WadManager created"), Manager);

    // Adding non-existent file should fail
    bool bResult = Manager->AddFile(TEXT("/nonexistent/doom.wad"));
    TestFalse(TEXT("Non-existent file fails"), bResult);

    // Empty manager finds no lumps
    int32 LumpIdx = Manager->FindLump(TEXT("PLAYPAL"));
    TestEqual(TEXT("No lumps in empty manager"), LumpIdx, -1);

    return true;
}

// ============================================================================
// Linedef Flags Tests (matching DOOM values)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomLinedefFlagsTest, "UnrealDoom.WAD.LinedefFlags",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomLinedefFlagsTest::RunTest(const FString& Parameters)
{
    // Original DOOM linedef flags
    constexpr int16 ML_BLOCKING = 1;
    constexpr int16 ML_BLOCKMONSTERS = 2;
    constexpr int16 ML_TWOSIDED = 4;
    constexpr int16 ML_DONTPEGTOP = 8;
    constexpr int16 ML_DONTPEGBOTTOM = 16;
    constexpr int16 ML_SECRET = 32;
    constexpr int16 ML_SOUNDBLOCK = 64;
    constexpr int16 ML_DONTDRAW = 128;
    constexpr int16 ML_MAPPED = 256;

    // Test typical wall configurations
    // Solid wall: blocking, one-sided
    int16 SolidWall = ML_BLOCKING;
    TestTrue(TEXT("Solid wall blocks"), (SolidWall & ML_BLOCKING) != 0);
    TestFalse(TEXT("Solid wall not two-sided"), (SolidWall & ML_TWOSIDED) != 0);

    // Door: two-sided, don't peg top
    int16 Door = ML_TWOSIDED | ML_DONTPEGTOP;
    TestTrue(TEXT("Door is two-sided"), (Door & ML_TWOSIDED) != 0);
    TestTrue(TEXT("Door don't peg top"), (Door & ML_DONTPEGTOP) != 0);

    // Secret wall: blocking, secret
    int16 SecretWall = ML_BLOCKING | ML_SECRET;
    TestTrue(TEXT("Secret wall is secret"), (SecretWall & ML_SECRET) != 0);

    // Hidden line: don't draw on automap
    int16 HiddenLine = ML_TWOSIDED | ML_DONTDRAW;
    TestTrue(TEXT("Hidden line not drawn"), (HiddenLine & ML_DONTDRAW) != 0);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
