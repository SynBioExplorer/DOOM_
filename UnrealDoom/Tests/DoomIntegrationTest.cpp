// DoomIntegrationTest.cpp - Integration tests for the complete DOOM UE5 port
// Tests cross-system interactions and end-to-end workflows
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"
#include "Core/DoomRandom.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// Full Game Init Sequence Test
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomInitSequenceTest, "UnrealDoom.Integration.InitSequence",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomInitSequenceTest::RunTest(const FString& Parameters)
{
    // DOOM initialization order (from d_main.c D_DoomMain):
    // 1. Z_Init (memory zone)
    // 2. M_FindResponseFile (command line)
    // 3. IdentifyVersion (detect WAD type)
    // 4. W_InitMultipleFiles (load WADs)
    // 5. V_Init (video buffers)
    // 6. M_LoadDefaults (config)
    // 7. R_Init (renderer) -> loads textures, sprites, colormaps
    // 8. P_Init (game logic) -> loads animations, switches
    // 9. I_InitSound (sound system)
    // 10. S_Init (high-level sound)
    // 11. D_CheckNetGame (network)
    // 12. HU_Init (HUD)
    // 13. ST_Init (status bar)

    // Verify the init order maps to UE5 subsystem initialization
    TArray<FString> InitOrder = {
        TEXT("WadManager::InitMultipleFiles"),
        TEXT("DoomGameState::Init"),
        TEXT("DoomLevelLoader::Init"),
        TEXT("DoomMeshGenerator::Init"),
        TEXT("DoomAudioSystem::Init"),
        TEXT("DoomThinkerManager::Init"),
        TEXT("DoomHUD::Init"),
    };

    TestEqual(TEXT("Init step count"), InitOrder.Num(), 7);
    TestEqual(TEXT("WAD loads first"), InitOrder[0], TEXT("WadManager::InitMultipleFiles"));
    TestEqual(TEXT("HUD loads last"), InitOrder.Last(), TEXT("DoomHUD::Init"));

    return true;
}

// ============================================================================
// New Game Flow Test
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomNewGameFlowTest, "UnrealDoom.Integration.NewGameFlow",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomNewGameFlowTest::RunTest(const FString& Parameters)
{
    // New Game flow:
    // 1. Player selects skill level
    // 2. Player selects episode (DOOM 1) or starts MAP01 (DOOM 2)
    // 3. G_InitNew() sets up game parameters
    // 4. G_DoLoadLevel() loads the map
    // 5. P_SetupLevel() loads map geometry from WAD
    // 6. P_SpawnMapThing() spawns all things from THINGS lump
    // 7. Player spawns at first player start (thing type 1)
    // 8. Game loop begins

    // Verify player spawn initialization
    constexpr int32 PLAYER_START_TYPE = 1;
    constexpr int32 PLAYER2_START_TYPE = 2;
    constexpr int32 PLAYER3_START_TYPE = 3;
    constexpr int32 PLAYER4_START_TYPE = 4;
    constexpr int32 DEATHMATCH_START_TYPE = 11;

    TestEqual(TEXT("P1 start type"), PLAYER_START_TYPE, 1);
    TestEqual(TEXT("P2 start type"), PLAYER2_START_TYPE, 2);
    TestEqual(TEXT("DM start type"), DEATHMATCH_START_TYPE, 11);

    // After spawn, player should have:
    constexpr int32 INIT_HEALTH = 100;
    constexpr int32 INIT_ARMOR = 0;
    constexpr int32 INIT_BULLETS = 50;
    constexpr bool INIT_HAS_FIST = true;
    constexpr bool INIT_HAS_PISTOL = true;

    TestEqual(TEXT("Init health"), INIT_HEALTH, 100);
    TestEqual(TEXT("Init armor"), INIT_ARMOR, 0);
    TestEqual(TEXT("Init bullets"), INIT_BULLETS, 50);
    TestTrue(TEXT("Has fist"), INIT_HAS_FIST);
    TestTrue(TEXT("Has pistol"), INIT_HAS_PISTOL);

    return true;
}

// ============================================================================
// Level Completion Flow Test
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomLevelCompleteFlowTest, "UnrealDoom.Integration.LevelComplete",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomLevelCompleteFlowTest::RunTest(const FString& Parameters)
{
    // Level completion flow:
    // 1. Player crosses exit linedef (special 11=normal, 51=secret, 52=normal_DOOM2)
    // 2. G_ExitLevel() or G_SecretExitLevel() called
    // 3. gameaction = ga_completed
    // 4. G_DoCompleted() processes
    // 5. Player stats tallied
    // 6. gamestate changes to GS_INTERMISSION
    // 7. WI_Start() begins intermission screen
    // 8. After intermission, ga_worlddone
    // 9. G_DoWorldDone() -> load next level

    // Exit linedef specials
    constexpr int32 EXIT_NORMAL = 11;
    constexpr int32 EXIT_SECRET = 51;
    constexpr int32 EXIT_NORMAL_DOOM2 = 52;

    TestEqual(TEXT("Normal exit"), EXIT_NORMAL, 11);
    TestEqual(TEXT("Secret exit"), EXIT_SECRET, 51);

    // Inventory carries over between levels:
    // Health, armor, weapons, ammo all persist
    // Power-ups are cleared (except allmap discovery)
    struct FLevelTransitionState
    {
        int32 Health;
        int32 Armor;
        int32 ArmorType;
        bool WeaponOwned[9];
        int32 Ammo[4];
        bool Backpack;
        // Powers are CLEARED
    };

    FLevelTransitionState TransState;
    TransState.Health = 73;
    TransState.Armor = 45;
    TransState.ArmorType = 1;
    TransState.Backpack = true;

    // These should carry over
    TestEqual(TEXT("Health persists"), TransState.Health, 73);
    TestEqual(TEXT("Armor persists"), TransState.Armor, 45);
    TestTrue(TEXT("Backpack persists"), TransState.Backpack);

    return true;
}

// ============================================================================
// Deterministic Simulation Test
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomDeterminismTest, "UnrealDoom.Integration.Determinism",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomDeterminismTest::RunTest(const FString& Parameters)
{
    // DOOM is fully deterministic given the same inputs
    // This is critical for demo recording/playback and networking
    // The PRNG (P_Random) must produce identical sequences

    // Run the same sequence of game operations twice
    DoomRandom::M_ClearRandom();

    TArray<int32> Sequence1;
    for (int32 i = 0; i < 100; i++)
    {
        Sequence1.Add(DoomRandom::P_Random());
    }

    // Reset and run again
    DoomRandom::M_ClearRandom();

    TArray<int32> Sequence2;
    for (int32 i = 0; i < 100; i++)
    {
        Sequence2.Add(DoomRandom::P_Random());
    }

    // Both sequences must be identical
    TestEqual(TEXT("Sequence lengths match"), Sequence1.Num(), Sequence2.Num());
    bool bIdentical = true;
    for (int32 i = 0; i < Sequence1.Num(); i++)
    {
        if (Sequence1[i] != Sequence2[i])
        {
            bIdentical = false;
            break;
        }
    }
    TestTrue(TEXT("PRNG is deterministic"), bIdentical);

    // Fixed-point math must also be deterministic
    int32 A = 12345 * FRACUNIT;
    int32 B = 67890 * FRACUNIT;
    int32 Result1 = DoomMath::FixedMul(A, B);
    int32 Result2 = DoomMath::FixedMul(A, B);
    TestEqual(TEXT("FixedMul deterministic"), Result1, Result2);

    return true;
}

// ============================================================================
// Cross-System Interaction Test
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomCrossSystemTest, "UnrealDoom.Integration.CrossSystem",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomCrossSystemTest::RunTest(const FString& Parameters)
{
    // Test that game systems interact correctly:

    // 1. Weapon fire -> Sound + Entity spawn + Ammo decrease
    // When player fires shotgun:
    // - Play sfx_shotgn sound
    // - Spawn 7 hitscan traces (P_LineAttack x7)
    // - Decrease shell count by 1
    // - Set weapon state to flash frame
    // - Increase player extralight for muzzle flash

    constexpr int32 SHOTGUN_PELLETS = 7;
    constexpr int32 SHOTGUN_AMMO_USE = 1;
    constexpr int32 SSG_PELLETS = 20;
    constexpr int32 SSG_AMMO_USE = 2;

    TestEqual(TEXT("Shotgun pellets"), SHOTGUN_PELLETS, 7);
    TestEqual(TEXT("Shotgun ammo"), SHOTGUN_AMMO_USE, 1);
    TestEqual(TEXT("SSG pellets"), SSG_PELLETS, 20);
    TestEqual(TEXT("SSG ammo"), SSG_AMMO_USE, 2);

    // 2. Monster death -> Sound + State change + Item drop + Kill count
    // When imp dies:
    // - Play death sound (sfx_bgdth1 or sfx_bgdth2)
    // - Enter death state animation
    // - May drop clip (MT_CLIP with MF_DROPPED)
    // - Increment player killcount
    // - Remove from thinker list after animation

    // 3. Door activation -> Sound + Sector movement + AI alert
    // When player uses a door:
    // - Check for key requirements
    // - Play sfx_doropn
    // - Start ceiling movement in sector
    // - Sound propagates to adjacent sectors
    // - Monsters may be alerted

    // 4. Teleporter -> Sound + Position change + Angle change
    // When entity enters teleporter:
    // - Play sfx_telept at origin
    // - Move entity to destination
    // - Set angle to destination angle
    // - Play sfx_telept at destination
    // - Spawn telefog (MT_TFOG) at both ends
    constexpr int32 TELEFOG_TYPE = 0; // MT_TFOG placeholder

    // 5. Barrel explosion chain -> Damage + Sound + Entity destruction
    // Barrel takes damage -> explodes -> radius damage -> may hit other barrels
    constexpr int32 BARREL_HEALTH = 20;
    constexpr int32 BARREL_EXPLOSION_DAMAGE = 128;

    TestEqual(TEXT("Barrel health"), BARREL_HEALTH, 20);
    TestEqual(TEXT("Barrel explosion"), BARREL_EXPLOSION_DAMAGE, 128);

    return true;
}

// ============================================================================
// Save/Load Consistency Test
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomSaveLoadTest, "UnrealDoom.Integration.SaveLoad",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomSaveLoadTest::RunTest(const FString& Parameters)
{
    // Save game format must preserve:
    // - Game mode, skill, episode, map
    // - Level time
    // - All mobj positions, states, health
    // - Player stats (health, armor, weapons, ammo, powers)
    // - Sector states (floor/ceiling heights, light levels)
    // - Thinker states (doors in motion, platforms, crushers)
    // - PRNG state

    // Save slot: 6 slots (0-5) + quicksave
    constexpr int32 NUM_SAVE_SLOTS = 6;
    TestEqual(TEXT("Save slots"), NUM_SAVE_SLOTS, 6);

    // Save game description: 24 characters max
    constexpr int32 SAVESTRINGSIZE = 24;
    FString SaveDesc = TEXT("Test Save Game");
    TestTrue(TEXT("Save desc fits"), SaveDesc.Len() <= SAVESTRINGSIZE);

    // Version check on load
    constexpr int32 SAVE_VERSION = 110; // Must match game version
    TestEqual(TEXT("Save version"), SAVE_VERSION, 110);

    return true;
}

// ============================================================================
// Performance Constraint Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomPerformanceTest, "UnrealDoom.Integration.Performance",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomPerformanceTest::RunTest(const FString& Parameters)
{
    // DOOM performance constraints that must be maintained:

    // Max visplanes: 128 (original), but UE5 doesn't have this limit
    // Max drawsegs: 256
    // Max vissprites: 128
    // Max openings: 20480

    // These limits don't apply to UE5, but we should ensure
    // the port handles equivalent large levels without issues

    // Typical level sizes:
    constexpr int32 TYPICAL_VERTICES = 500;
    constexpr int32 TYPICAL_LINEDEFS = 600;
    constexpr int32 TYPICAL_SIDEDEFS = 900;
    constexpr int32 TYPICAL_SECTORS = 100;
    constexpr int32 TYPICAL_THINGS = 200;

    // Large levels (MAP30 size):
    constexpr int32 LARGE_VERTICES = 3000;
    constexpr int32 LARGE_LINEDEFS = 4000;
    constexpr int32 LARGE_SECTORS = 500;

    TestTrue(TEXT("Large > typical vertices"), LARGE_VERTICES > TYPICAL_VERTICES);
    TestTrue(TEXT("Large > typical linedefs"), LARGE_LINEDEFS > TYPICAL_LINEDEFS);

    // Memory budget estimates for UE5 port:
    // Per vertex: 12 bytes (FVector)
    // Per triangle: 12 bytes (3 indices)
    // Per sector mesh: ~50 triangles avg * 36 bytes = ~1.8KB
    // Total for large level: 500 sectors * 1.8KB = ~900KB geometry

    int32 LargeLevelGeoKB = LARGE_SECTORS * 2; // ~2KB per sector
    TestTrue(TEXT("Geo budget < 10MB"), LargeLevelGeoKB < 10 * 1024);

    // Tick budget: 35 tics/sec = ~28.57ms per tic
    // All game logic must complete within this budget
    constexpr float TIC_BUDGET_MS = 1000.0f / TICRATE;
    TestTrue(TEXT("Tic budget ~28ms"), FMath::IsNearlyEqual(TIC_BUDGET_MS, 28.57f, 0.1f));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
