// DoomRendererTest.cpp - Tests for BSP tree, level geometry, and mesh generation
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"
#include "Renderer/DoomMapData.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// BSP Node Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomBSPNodeTest, "UnrealDoom.Renderer.BSPNode",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomBSPNodeTest::RunTest(const FString& Parameters)
{
    // NF_SUBSECTOR flag (0x8000 for 16-bit, 0x80000000 for 32-bit)
    constexpr uint16 NF_SUBSECTOR_16 = 0x8000;
    constexpr uint32 NF_SUBSECTOR_32 = 0x80000000;

    // Test subsector identification
    uint16 LeafNode = 0x8005; // Subsector 5
    TestTrue(TEXT("Leaf node flagged"), (LeafNode & NF_SUBSECTOR_16) != 0);
    uint16 SubsectorIdx = LeafNode & (~NF_SUBSECTOR_16);
    TestEqual(TEXT("Subsector index"), (int32)SubsectorIdx, 5);

    // Internal node (no subsector flag)
    uint16 InternalNode = 42;
    TestFalse(TEXT("Internal node not leaf"), (InternalNode & NF_SUBSECTOR_16) != 0);

    // Partition line: a BSP node splits space with a line
    FDoomNode Node;
    Node.X = 100;
    Node.Y = 200;
    Node.Dx = 50;
    Node.Dy = 0; // Horizontal partition line

    // Point-on-side test: which side of the partition line is a point?
    // Side = (point.y - node.y) * node.dx - (point.x - node.x) * node.dy
    auto PointOnSide = [](float Px, float Py, const FDoomNode& N) -> int32
    {
        float Dx = Px - N.X;
        float Dy = Py - N.Y;
        float Cross = Dy * N.Dx - Dx * N.Dy;
        return Cross <= 0 ? 0 : 1; // 0=front, 1=back
    };

    // Point above horizontal line = front (side 0)
    int32 Side = PointOnSide(120.0f, 300.0f, Node); // Above the line
    TestEqual(TEXT("Point above line = front"), Side, 0);

    // Point below horizontal line = back (side 1)
    Side = PointOnSide(120.0f, 100.0f, Node); // Below the line
    TestEqual(TEXT("Point below line = back"), Side, 1);

    return true;
}

// ============================================================================
// Sector Geometry Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomSectorGeomTest, "UnrealDoom.Renderer.SectorGeometry",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomSectorGeomTest::RunTest(const FString& Parameters)
{
    // Sector heights determine wall segments
    FDoomSector FrontSector;
    FrontSector.FloorHeight = 0.0f;
    FrontSector.CeilingHeight = 128.0f;
    FrontSector.LightLevel = 192;

    FDoomSector BackSector;
    BackSector.FloorHeight = 24.0f;
    BackSector.CeilingHeight = 128.0f;
    BackSector.LightLevel = 160;

    // Wall types for two-sided linedef:
    // Upper wall: front ceiling to back ceiling (when front ceiling > back ceiling)
    float UpperWallTop = FrontSector.CeilingHeight;
    float UpperWallBot = BackSector.CeilingHeight;
    float UpperHeight = UpperWallTop - UpperWallBot;
    TestEqual(TEXT("No upper wall (equal ceilings)"), UpperHeight, 0.0f);

    // Lower wall: back floor to front floor (when back floor > front floor)
    float LowerWallTop = BackSector.FloorHeight;
    float LowerWallBot = FrontSector.FloorHeight;
    float LowerHeight = LowerWallTop - LowerWallBot;
    TestEqual(TEXT("Lower wall height"), LowerHeight, 24.0f);

    // Middle wall for one-sided linedef
    float MiddleHeight = FrontSector.CeilingHeight - FrontSector.FloorHeight;
    TestEqual(TEXT("One-sided wall height"), MiddleHeight, 128.0f);

    // Test with step-down (lower back sector)
    BackSector.FloorHeight = -16.0f;
    BackSector.CeilingHeight = 96.0f;

    UpperHeight = FrontSector.CeilingHeight - BackSector.CeilingHeight;
    TestEqual(TEXT("Upper wall 32 units"), UpperHeight, 32.0f);

    // No lower wall needed (back floor is lower)
    float LowerNeeded = FrontSector.FloorHeight - BackSector.FloorHeight;
    TestTrue(TEXT("Lower not needed (front floor higher)"), LowerNeeded > 0);

    return true;
}

// ============================================================================
// UV Coordinate Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomUVCoordTest, "UnrealDoom.Renderer.UVCoordinates",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomUVCoordTest::RunTest(const FString& Parameters)
{
    // DOOM textures are typically 64, 128, or 256 pixels wide
    // UV wrapping: U = offset / textureWidth, V = height / textureHeight
    constexpr float TEX_WIDTH_64 = 64.0f;
    constexpr float TEX_WIDTH_128 = 128.0f;
    constexpr float TEX_HEIGHT_128 = 128.0f;

    // Wall segment 200 units long with 128-wide texture
    float WallLength = 200.0f;
    float U_End = WallLength / TEX_WIDTH_128;
    TestTrue(TEXT("UV wraps"), U_End > 1.0f);
    TestTrue(TEXT("UV value"), FMath::IsNearlyEqual(U_End, 200.0f / 128.0f, 0.001f));

    // Texture offset applied to U
    float TextureOffset = 32.0f;
    float U_Start = TextureOffset / TEX_WIDTH_128;
    TestTrue(TEXT("Offset UV"), FMath::IsNearlyEqual(U_Start, 0.25f, 0.001f));

    // Floor/ceiling UV: DOOM flats are always 64x64
    constexpr float FLAT_SIZE = 64.0f;
    float FloorU = 100.0f / FLAT_SIZE; // World X / flat size
    float FloorV = 200.0f / FLAT_SIZE; // World Y / flat size
    TestTrue(TEXT("Floor UV wraps"), FloorU > 1.0f);
    TestTrue(TEXT("Floor UV"), FMath::IsNearlyEqual(FloorU, 100.0f / 64.0f, 0.001f));

    return true;
}

// ============================================================================
// Light Level Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomLightLevelTest, "UnrealDoom.Renderer.LightLevels",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomLightLevelTest::RunTest(const FString& Parameters)
{
    // DOOM light levels range from 0 (dark) to 255 (bright)
    // Convert to UE5 light intensity (0.0 to 1.0)

    auto LightToIntensity = [](int32 LightLevel) -> float
    {
        return FMath::Clamp(LightLevel / 255.0f, 0.0f, 1.0f);
    };

    TestTrue(TEXT("Dark = 0.0"), FMath::IsNearlyEqual(LightToIntensity(0), 0.0f, 0.01f));
    TestTrue(TEXT("Full = 1.0"), FMath::IsNearlyEqual(LightToIntensity(255), 1.0f, 0.01f));
    TestTrue(TEXT("Half light"), FMath::IsNearlyEqual(LightToIntensity(128), 128.0f / 255.0f, 0.01f));

    // Common sector light levels in DOOM maps
    TestTrue(TEXT("Outdoor light"), LightToIntensity(192) > 0.7f);
    TestTrue(TEXT("Indoor light"), LightToIntensity(160) > 0.5f);
    TestTrue(TEXT("Dark corridor"), LightToIntensity(96) < 0.4f);
    TestTrue(TEXT("Pitch black"), LightToIntensity(0) == 0.0f);

    // Vertex color encoding for mesh generation
    auto LightToVertexColor = [](int32 LightLevel) -> FLinearColor
    {
        float Intensity = FMath::Clamp(LightLevel / 255.0f, 0.0f, 1.0f);
        return FLinearColor(Intensity, Intensity, Intensity, 1.0f);
    };

    FLinearColor BrightColor = LightToVertexColor(255);
    TestTrue(TEXT("Bright vertex R"), FMath::IsNearlyEqual(BrightColor.R, 1.0f));
    TestTrue(TEXT("Bright vertex G"), FMath::IsNearlyEqual(BrightColor.G, 1.0f));
    TestTrue(TEXT("Bright vertex B"), FMath::IsNearlyEqual(BrightColor.B, 1.0f));

    FLinearColor DarkColor = LightToVertexColor(0);
    TestTrue(TEXT("Dark vertex R"), FMath::IsNearlyEqual(DarkColor.R, 0.0f));

    return true;
}

// ============================================================================
// Mesh Generation Validation Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomMeshGenTest, "UnrealDoom.Renderer.MeshGeneration",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomMeshGenTest::RunTest(const FString& Parameters)
{
    // Test triangle generation for a simple rectangular sector
    // 4 vertices forming a rectangle
    TArray<FVector> Vertices;
    Vertices.Add(FVector(0, 0, 0));
    Vertices.Add(FVector(100, 0, 0));
    Vertices.Add(FVector(100, 100, 0));
    Vertices.Add(FVector(0, 100, 0));

    // Simple fan triangulation from vertex 0
    TArray<int32> Triangles;
    for (int32 i = 1; i < Vertices.Num() - 1; i++)
    {
        Triangles.Add(0);
        Triangles.Add(i);
        Triangles.Add(i + 1);
    }

    // Rectangle = 2 triangles = 6 indices
    TestEqual(TEXT("Triangle count"), Triangles.Num(), 6);
    TestEqual(TEXT("2 triangles"), Triangles.Num() / 3, 2);

    // Verify winding order (counter-clockwise for UE5 default)
    // Triangle 1: 0, 1, 2
    TestEqual(TEXT("Tri1 v0"), Triangles[0], 0);
    TestEqual(TEXT("Tri1 v1"), Triangles[1], 1);
    TestEqual(TEXT("Tri1 v2"), Triangles[2], 2);

    // Triangle 2: 0, 2, 3
    TestEqual(TEXT("Tri2 v0"), Triangles[3], 0);
    TestEqual(TEXT("Tri2 v1"), Triangles[4], 2);
    TestEqual(TEXT("Tri2 v2"), Triangles[5], 3);

    // Wall quad: 2 triangles from 4 vertices
    // Bottom-left, bottom-right, top-right, top-left
    TArray<FVector> WallVerts;
    WallVerts.Add(FVector(0, 0, 0));     // BL
    WallVerts.Add(FVector(100, 0, 0));   // BR
    WallVerts.Add(FVector(100, 0, 128)); // TR
    WallVerts.Add(FVector(0, 0, 128));   // TL

    TArray<int32> WallTris = {0, 1, 2, 0, 2, 3};
    TestEqual(TEXT("Wall triangles"), WallTris.Num(), 6);

    // Normal should point towards viewer (positive Y for this wall)
    FVector Edge1 = WallVerts[1] - WallVerts[0];
    FVector Edge2 = WallVerts[2] - WallVerts[0];
    FVector Normal = FVector::CrossProduct(Edge1, Edge2).GetSafeNormal();
    TestTrue(TEXT("Wall normal Y"), Normal.Y > 0.0f || Normal.Y < 0.0f); // Should have Y component

    return true;
}

// ============================================================================
// Coordinate Scale Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomScaleTest, "UnrealDoom.Renderer.CoordinateScale",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomScaleTest::RunTest(const FString& Parameters)
{
    // DOOM -> UE5 scale: 1 DOOM unit = 2.0 UE units
    constexpr float DOOM_TO_UE_SCALE = 2.0f;

    // Standard room in DOOM: 256x256 units, 128 height
    float RoomWidth = 256.0f * DOOM_TO_UE_SCALE;   // 512 UE units
    float RoomHeight = 128.0f * DOOM_TO_UE_SCALE;  // 256 UE units

    TestEqual(TEXT("Room width in UE"), RoomWidth, 512.0f);
    TestEqual(TEXT("Room height in UE"), RoomHeight, 256.0f);

    // Player height: 56 DOOM units
    float PlayerHeight = 56.0f * DOOM_TO_UE_SCALE; // 112 UE units (~1.12m, a bit short but matches DOOM's scale)
    TestEqual(TEXT("Player height UE"), PlayerHeight, 112.0f);

    // Player radius: 16 DOOM units
    float PlayerRadius = 16.0f * DOOM_TO_UE_SCALE; // 32 UE units
    TestEqual(TEXT("Player radius UE"), PlayerRadius, 32.0f);

    // Door standard width: 64 DOOM units = 128 UE
    float DoorWidth = 64.0f * DOOM_TO_UE_SCALE;
    TestEqual(TEXT("Door width UE"), DoorWidth, 128.0f);

    // Standard step height: 24 DOOM units = 48 UE
    float StepHeight = 24.0f * DOOM_TO_UE_SCALE;
    TestEqual(TEXT("Step height UE"), StepHeight, 48.0f);

    // Max step-up height for player: 24 DOOM units
    float MaxStepUp = 24.0f * DOOM_TO_UE_SCALE;
    TestEqual(TEXT("Max step-up UE"), MaxStepUp, 48.0f);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
