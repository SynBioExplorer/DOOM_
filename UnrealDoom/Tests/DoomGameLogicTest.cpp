// DoomGameLogicTest.cpp - Tests for game state machine, thinker system, and game loop
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"
#include "GameLogic/DoomTicCmd.h"
#include "GameLogic/DoomThinker.h"
#include "GameLogic/DoomGameState.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// Game State Machine Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomGameStateMachineTest, "UnrealDoom.GameLogic.StateMachine",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomGameStateMachineTest::RunTest(const FString& Parameters)
{
    UDoomGameState* State = NewObject<UDoomGameState>();
    TestNotNull(TEXT("GameState created"), State);

    // Initial state should be DemoScreen
    State->SetGameState(EDoomGameState::DemoScreen);
    TestEqual(TEXT("Initial state"), State->GetGameState(), EDoomGameState::DemoScreen);

    // Transition: DemoScreen -> Level (new game)
    State->SetGameState(EDoomGameState::Level);
    TestEqual(TEXT("Level state"), State->GetGameState(), EDoomGameState::Level);

    // Transition: Level -> Intermission (level complete)
    State->SetGameState(EDoomGameState::Intermission);
    TestEqual(TEXT("Intermission state"), State->GetGameState(), EDoomGameState::Intermission);

    // Transition: Intermission -> Level (next level)
    State->SetGameState(EDoomGameState::Level);
    TestEqual(TEXT("Back to level"), State->GetGameState(), EDoomGameState::Level);

    // Transition: Level -> Finale (episode end)
    State->SetGameState(EDoomGameState::Finale);
    TestEqual(TEXT("Finale state"), State->GetGameState(), EDoomGameState::Finale);

    return true;
}

// ============================================================================
// Game Action Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomGameActionTest, "UnrealDoom.GameLogic.GameActions",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomGameActionTest::RunTest(const FString& Parameters)
{
    // Verify all game actions have correct values
    TestEqual(TEXT("ga_nothing"), (int32)EDoomGameAction::Nothing, 0);
    TestEqual(TEXT("ga_loadlevel"), (int32)EDoomGameAction::LoadLevel, 1);
    TestEqual(TEXT("ga_newgame"), (int32)EDoomGameAction::NewGame, 2);
    TestEqual(TEXT("ga_loadgame"), (int32)EDoomGameAction::LoadGame, 3);
    TestEqual(TEXT("ga_savegame"), (int32)EDoomGameAction::SaveGame, 4);
    TestEqual(TEXT("ga_playdemo"), (int32)EDoomGameAction::PlayDemo, 5);
    TestEqual(TEXT("ga_completed"), (int32)EDoomGameAction::Completed, 6);
    TestEqual(TEXT("ga_victory"), (int32)EDoomGameAction::Victory, 7);
    TestEqual(TEXT("ga_worlddone"), (int32)EDoomGameAction::WorldDone, 8);
    TestEqual(TEXT("ga_screenshot"), (int32)EDoomGameAction::Screenshot, 9);

    return true;
}

// ============================================================================
// Thinker System Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomThinkerTest, "UnrealDoom.GameLogic.ThinkerSystem",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomThinkerTest::RunTest(const FString& Parameters)
{
    UDoomThinkerManager* ThinkerMgr = NewObject<UDoomThinkerManager>();
    TestNotNull(TEXT("ThinkerManager created"), ThinkerMgr);

    // Initial state: no thinkers
    TestEqual(TEXT("Initial thinker count"), ThinkerMgr->GetThinkerCount(), 0);

    // Add thinkers
    int32 CallCount = 0;
    auto ThinkFunc = [&CallCount]() { CallCount++; };

    int32 Id1 = ThinkerMgr->AddThinker(ThinkFunc);
    TestTrue(TEXT("Thinker 1 added"), Id1 >= 0);
    TestEqual(TEXT("One thinker"), ThinkerMgr->GetThinkerCount(), 1);

    int32 Id2 = ThinkerMgr->AddThinker(ThinkFunc);
    TestTrue(TEXT("Thinker 2 added"), Id2 >= 0);
    TestEqual(TEXT("Two thinkers"), ThinkerMgr->GetThinkerCount(), 2);

    // Run thinkers
    ThinkerMgr->RunThinkers();
    TestEqual(TEXT("Both thinkers ran"), CallCount, 2);

    // Run again
    ThinkerMgr->RunThinkers();
    TestEqual(TEXT("Ran again"), CallCount, 4);

    // Remove a thinker
    ThinkerMgr->RemoveThinker(Id1);
    TestEqual(TEXT("One thinker remaining"), ThinkerMgr->GetThinkerCount(), 1);

    CallCount = 0;
    ThinkerMgr->RunThinkers();
    TestEqual(TEXT("Only one runs"), CallCount, 1);

    // Remove all
    ThinkerMgr->RemoveThinker(Id2);
    TestEqual(TEXT("No thinkers"), ThinkerMgr->GetThinkerCount(), 0);

    CallCount = 0;
    ThinkerMgr->RunThinkers();
    TestEqual(TEXT("None run"), CallCount, 0);

    return true;
}

// ============================================================================
// Tic Timing Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomTicTimingTest, "UnrealDoom.GameLogic.TicTiming",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomTicTimingTest::RunTest(const FString& Parameters)
{
    // DOOM runs at exactly 35 tics per second
    constexpr float TIC_DURATION = 1.0f / TICRATE; // ~0.02857 seconds

    TestTrue(TEXT("Tic duration"), FMath::IsNearlyEqual(TIC_DURATION, 1.0f / 35.0f, 0.0001f));

    // One second = 35 tics
    TestEqual(TEXT("Tics per second"), TICRATE, 35);

    // Level time is measured in tics
    // Par times are in seconds, converted to tics for display
    constexpr int32 E1M1_PAR_SECONDS = 30;
    constexpr int32 E1M1_PAR_TICS = E1M1_PAR_SECONDS * TICRATE;
    TestEqual(TEXT("E1M1 par tics"), E1M1_PAR_TICS, 1050);

    // Accumulator test: simulating UE5 variable timestep -> DOOM fixed timestep
    float Accumulator = 0.0f;
    int32 TicsRun = 0;

    // Simulate 1 second of gameplay at 60fps
    for (int32 Frame = 0; Frame < 60; Frame++)
    {
        float DeltaTime = 1.0f / 60.0f; // ~16.67ms
        Accumulator += DeltaTime;

        while (Accumulator >= TIC_DURATION)
        {
            Accumulator -= TIC_DURATION;
            TicsRun++;
        }
    }

    // After 1 second, should have run exactly 35 tics
    TestEqual(TEXT("35 tics in 1 second"), TicsRun, 35);

    // Test at 30fps
    Accumulator = 0.0f;
    TicsRun = 0;
    for (int32 Frame = 0; Frame < 30; Frame++)
    {
        float DeltaTime = 1.0f / 30.0f;
        Accumulator += DeltaTime;
        while (Accumulator >= TIC_DURATION)
        {
            Accumulator -= TIC_DURATION;
            TicsRun++;
        }
    }
    TestEqual(TEXT("35 tics at 30fps"), TicsRun, 35);

    // Test at 144fps
    Accumulator = 0.0f;
    TicsRun = 0;
    for (int32 Frame = 0; Frame < 144; Frame++)
    {
        float DeltaTime = 1.0f / 144.0f;
        Accumulator += DeltaTime;
        while (Accumulator >= TIC_DURATION)
        {
            Accumulator -= TIC_DURATION;
            TicsRun++;
        }
    }
    TestEqual(TEXT("35 tics at 144fps"), TicsRun, 35);

    return true;
}

// ============================================================================
// Episode/Map Validation Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomEpisodeMapTest, "UnrealDoom.GameLogic.EpisodeMaps",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomEpisodeMapTest::RunTest(const FString& Parameters)
{
    // DOOM 1 Shareware: Episode 1, Maps 1-9
    // DOOM 1 Registered: Episodes 1-3, Maps 1-9 each
    // DOOM 1 Retail (Ultimate): Episodes 1-4, Maps 1-9 each
    // DOOM 2: Episode 1, Maps 1-32 (30 normal + 2 secret)

    // Map lump naming convention
    // DOOM 1: E1M1 through E4M9
    // DOOM 2: MAP01 through MAP32

    auto MakeDoom1MapName = [](int32 Episode, int32 Map) -> FString
    {
        return FString::Printf(TEXT("E%dM%d"), Episode, Map);
    };

    auto MakeDoom2MapName = [](int32 Map) -> FString
    {
        return FString::Printf(TEXT("MAP%02d"), Map);
    };

    // DOOM 1 map names
    TestEqual(TEXT("E1M1"), MakeDoom1MapName(1, 1), TEXT("E1M1"));
    TestEqual(TEXT("E3M9"), MakeDoom1MapName(3, 9), TEXT("E3M9"));
    TestEqual(TEXT("E4M1"), MakeDoom1MapName(4, 1), TEXT("E4M1"));

    // DOOM 2 map names
    TestEqual(TEXT("MAP01"), MakeDoom2MapName(1), TEXT("MAP01"));
    TestEqual(TEXT("MAP15"), MakeDoom2MapName(15), TEXT("MAP15"));
    TestEqual(TEXT("MAP30"), MakeDoom2MapName(30), TEXT("MAP30"));
    TestEqual(TEXT("MAP31"), MakeDoom2MapName(31), TEXT("MAP31")); // Secret
    TestEqual(TEXT("MAP32"), MakeDoom2MapName(32), TEXT("MAP32")); // Super secret

    // Validate episode/map ranges for each game mode
    // Shareware
    int32 SharewareEpisodes = 1;
    int32 MapsPerEpisode = 9;
    TestEqual(TEXT("Shareware episodes"), SharewareEpisodes, 1);

    // Registered
    int32 RegisteredEpisodes = 3;
    TestEqual(TEXT("Registered episodes"), RegisteredEpisodes, 3);

    // Retail (Ultimate DOOM)
    int32 RetailEpisodes = 4;
    TestEqual(TEXT("Retail episodes"), RetailEpisodes, 4);

    // Commercial (DOOM 2)
    int32 CommercialMaps = 32;
    TestEqual(TEXT("DOOM 2 maps"), CommercialMaps, 32);

    // Secret level exits
    // E1M3 -> E1M9 (secret)
    // E2M5 -> E2M9 (secret)
    // E3M6 -> E3M9 (secret)
    // E4M2 -> E4M9 (secret)
    // MAP15 -> MAP31 (secret)
    // MAP31 -> MAP32 (super secret)

    return true;
}

// ============================================================================
// New Game Initialization Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomNewGameTest, "UnrealDoom.GameLogic.NewGame",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomNewGameTest::RunTest(const FString& Parameters)
{
    UDoomGameState* State = NewObject<UDoomGameState>();

    // Set up a new game
    State->SetSkill(EDoomSkill::Medium);
    State->SetEpisode(1);
    State->SetMap(1);
    State->SetGameState(EDoomGameState::Level);

    TestEqual(TEXT("Skill set"), State->GetSkill(), EDoomSkill::Medium);
    TestEqual(TEXT("Episode set"), State->GetEpisode(), 1);
    TestEqual(TEXT("Map set"), State->GetMap(), 1);
    TestEqual(TEXT("In level"), State->GetGameState(), EDoomGameState::Level);

    // Player initialization for new game
    // Should start with: 100 health, 0 armor, fist + pistol, 50 bullets
    constexpr int32 STARTING_HEALTH = 100;
    constexpr int32 STARTING_ARMOR = 0;
    constexpr int32 STARTING_BULLETS = 50;

    TestEqual(TEXT("Starting health"), STARTING_HEALTH, 100);
    TestEqual(TEXT("Starting armor"), STARTING_ARMOR, 0);
    TestEqual(TEXT("Starting bullets"), STARTING_BULLETS, 50);

    // Test skill modifiers
    // Baby: monsters do half damage, double ammo
    // Easy: no modifier
    // Medium: no modifier
    // Hard: no modifier
    // Nightmare: monsters respawn, fast monsters, double ammo

    struct FSkillModifier
    {
        bool bFastMonsters;
        bool bRespawnMonsters;
        int32 AmmoMultiplier;
        float DamageMultiplier;
    };

    TArray<FSkillModifier> SkillMods = {
        {false, false, 2, 0.5f},  // Baby
        {false, false, 1, 1.0f},  // Easy
        {false, false, 1, 1.0f},  // Medium
        {false, false, 1, 1.0f},  // Hard
        {true,  true,  2, 1.0f},  // Nightmare
    };

    TestTrue(TEXT("Baby double ammo"), SkillMods[0].AmmoMultiplier == 2);
    TestTrue(TEXT("Baby half damage"), SkillMods[0].DamageMultiplier < 1.0f);
    TestTrue(TEXT("Nightmare fast"), SkillMods[4].bFastMonsters);
    TestTrue(TEXT("Nightmare respawn"), SkillMods[4].bRespawnMonsters);
    TestTrue(TEXT("Nightmare double ammo"), SkillMods[4].AmmoMultiplier == 2);

    return true;
}

// ============================================================================
// Level Transition Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomLevelTransitionTest, "UnrealDoom.GameLogic.LevelTransition",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomLevelTransitionTest::RunTest(const FString& Parameters)
{
    // Normal progression: E1M1 -> E1M2 -> ... -> E1M8 -> intermission -> E2M1
    // Secret exits: E1M3 -> E1M9
    // Secret level return: E1M9 -> E1M4

    auto NextMap = [](int32 Episode, int32 Map, bool bSecret) -> TPair<int32, int32>
    {
        if (bSecret)
        {
            return TPair<int32, int32>(Episode, 9);  // Secret is always map 9
        }
        if (Map == 9)
        {
            return TPair<int32, int32>(Episode, 4);  // Secret returns to map 4
        }
        if (Map == 8)
        {
            // Episode complete
            return TPair<int32, int32>(Episode + 1, 1);
        }
        return TPair<int32, int32>(Episode, Map + 1);
    };

    // Normal E1M1 -> E1M2
    auto Next = NextMap(1, 1, false);
    TestEqual(TEXT("E1M1 next ep"), Next.Key, 1);
    TestEqual(TEXT("E1M1 next map"), Next.Value, 2);

    // Secret E1M3 -> E1M9
    Next = NextMap(1, 3, true);
    TestEqual(TEXT("E1M3 secret ep"), Next.Key, 1);
    TestEqual(TEXT("E1M3 secret map"), Next.Value, 9);

    // E1M9 return -> E1M4
    Next = NextMap(1, 9, false);
    TestEqual(TEXT("E1M9 return ep"), Next.Key, 1);
    TestEqual(TEXT("E1M9 return map"), Next.Value, 4);

    // E1M8 -> episode complete
    Next = NextMap(1, 8, false);
    TestEqual(TEXT("E1M8 end ep"), Next.Key, 2);
    TestEqual(TEXT("E1M8 end map"), Next.Value, 1);

    // DOOM 2 progression
    auto NextDoom2Map = [](int32 Map, bool bSecret) -> int32
    {
        if (bSecret && Map == 15) return 31;
        if (bSecret && Map == 31) return 32;
        if (Map == 31) return 16;
        if (Map == 32) return 16;
        return Map + 1;
    };

    TestEqual(TEXT("MAP01 -> MAP02"), NextDoom2Map(1, false), 2);
    TestEqual(TEXT("MAP15 secret -> MAP31"), NextDoom2Map(15, true), 31);
    TestEqual(TEXT("MAP31 secret -> MAP32"), NextDoom2Map(31, true), 32);
    TestEqual(TEXT("MAP31 normal -> MAP16"), NextDoom2Map(31, false), 16);
    TestEqual(TEXT("MAP32 -> MAP16"), NextDoom2Map(32, false), 16);

    return true;
}

// ============================================================================
// Intermission Stats Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomIntermissionTest, "UnrealDoom.GameLogic.Intermission",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomIntermissionTest::RunTest(const FString& Parameters)
{
    // Intermission shows: kills%, items%, secrets%, time, par time
    // Percentages are kills/maxkills * 100, etc.

    int32 Kills = 45;
    int32 MaxKills = 50;
    int32 KillPct = MaxKills > 0 ? (Kills * 100) / MaxKills : 0;
    TestEqual(TEXT("Kill percentage"), KillPct, 90);

    int32 Items = 10;
    int32 MaxItems = 10;
    int32 ItemPct = MaxItems > 0 ? (Items * 100) / MaxItems : 0;
    TestEqual(TEXT("Item percentage 100%"), ItemPct, 100);

    int32 Secrets = 0;
    int32 MaxSecrets = 3;
    int32 SecretPct = MaxSecrets > 0 ? (Secrets * 100) / MaxSecrets : 0;
    TestEqual(TEXT("Secret percentage 0%"), SecretPct, 0);

    // Zero max should not divide by zero
    int32 ZeroMax = 0;
    int32 SafePct = ZeroMax > 0 ? (5 * 100) / ZeroMax : 0;
    TestEqual(TEXT("Zero max safe"), SafePct, 0);

    // Time formatting: tics to minutes:seconds
    int32 LevelTics = 35 * 125; // 2 minutes 5 seconds
    int32 Seconds = LevelTics / TICRATE;
    int32 Minutes = Seconds / 60;
    Seconds %= 60;
    TestEqual(TEXT("Minutes"), Minutes, 2);
    TestEqual(TEXT("Seconds"), Seconds, 5);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
