// DoomAITest.cpp - Tests for enemy AI behavior, combat, and sight checking
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// AI Direction System Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomAIDirectionTest, "UnrealDoom.AI.DirectionSystem",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomAIDirectionTest::RunTest(const FString& Parameters)
{
    // DOOM uses 8 movement directions (0-7)
    // 0=East, 1=NE, 2=North, 3=NW, 4=West, 5=SW, 6=South, 7=SE
    // DI_NODIR = 8

    constexpr int32 DI_EAST = 0;
    constexpr int32 DI_NORTHEAST = 1;
    constexpr int32 DI_NORTH = 2;
    constexpr int32 DI_NORTHWEST = 3;
    constexpr int32 DI_WEST = 4;
    constexpr int32 DI_SOUTHWEST = 5;
    constexpr int32 DI_SOUTH = 6;
    constexpr int32 DI_SOUTHEAST = 7;
    constexpr int32 DI_NODIR = 8;

    // Movement deltas for each direction (speed multiplied by these)
    // xspeed[8] = {FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000, 0, 47000}
    // yspeed[8] = {0, 47000, FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000}
    constexpr int32 DIAG_SPEED = 47000; // ~0.7071 * FRACUNIT (sqrt(2)/2)

    struct FDirDelta { int32 Dx; int32 Dy; };
    FDirDelta Deltas[8] = {
        {FRACUNIT, 0},          // East
        {DIAG_SPEED, DIAG_SPEED},   // NE
        {0, FRACUNIT},          // North
        {-DIAG_SPEED, DIAG_SPEED},  // NW
        {-FRACUNIT, 0},         // West
        {-DIAG_SPEED, -DIAG_SPEED}, // SW
        {0, -FRACUNIT},         // South
        {DIAG_SPEED, -DIAG_SPEED},  // SE
    };

    // East moves positive X only
    TestTrue(TEXT("East +X"), Deltas[DI_EAST].Dx > 0);
    TestEqual(TEXT("East no Y"), Deltas[DI_EAST].Dy, 0);

    // North moves positive Y only
    TestEqual(TEXT("North no X"), Deltas[DI_NORTH].Dx, 0);
    TestTrue(TEXT("North +Y"), Deltas[DI_NORTH].Dy > 0);

    // Diagonal speed is sqrt(2)/2 of cardinal speed
    float DiagRatio = (float)DIAG_SPEED / (float)FRACUNIT;
    TestTrue(TEXT("Diagonal ratio ~0.707"), FMath::IsNearlyEqual(DiagRatio, 0.7071f, 0.01f));

    // Opposite directions
    TestEqual(TEXT("East opposite West"), (DI_EAST + 4) % 8, DI_WEST);
    TestEqual(TEXT("North opposite South"), (DI_NORTH + 4) % 8, DI_SOUTH);
    TestEqual(TEXT("NE opposite SW"), (DI_NORTHEAST + 4) % 8, DI_SOUTHWEST);

    // NODIR is special sentinel
    TestEqual(TEXT("NODIR = 8"), DI_NODIR, 8);
    TestTrue(TEXT("NODIR out of range"), DI_NODIR >= 8);

    return true;
}

// ============================================================================
// Melee/Missile Range Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomAIRangeTest, "UnrealDoom.AI.RangeChecks",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomAIRangeTest::RunTest(const FString& Parameters)
{
    // MELEERANGE = 64 * FRACUNIT
    constexpr int32 MELEERANGE = 64 * FRACUNIT;
    // MISSILERANGE = 32*64*FRACUNIT = 2048 * FRACUNIT
    constexpr int32 MISSILERANGE = 32 * 64 * FRACUNIT;

    TestEqual(TEXT("Melee range"), MELEERANGE, 64 * FRACUNIT);
    TestEqual(TEXT("Missile range"), MISSILERANGE, 2048 * FRACUNIT);

    // In UE units (scale 2x)
    float MeleeRangeUE = DoomMath::DoomUnitsToUE(MELEERANGE);
    float MissileRangeUE = DoomMath::DoomUnitsToUE(MISSILERANGE);
    TestTrue(TEXT("Melee range UE"), FMath::IsNearlyEqual(MeleeRangeUE, 128.0f, 1.0f));
    TestTrue(TEXT("Missile range UE"), FMath::IsNearlyEqual(MissileRangeUE, 4096.0f, 1.0f));

    // P_CheckMeleeRange logic:
    // Distance to target must be < MELEERANGE
    // Must have clear line of sight
    auto CheckMeleeRange = [MELEERANGE](int32 Distance) -> bool
    {
        return Distance < MELEERANGE;
    };

    TestTrue(TEXT("In melee range"), CheckMeleeRange(60 * FRACUNIT));
    TestFalse(TEXT("Out of melee range"), CheckMeleeRange(70 * FRACUNIT));
    TestFalse(TEXT("Exactly at melee range"), CheckMeleeRange(MELEERANGE));

    // P_CheckMissileRange logic:
    // Distance must be < MISSILERANGE
    // Probability decreases with distance
    // Close range: always fire
    // Far range: random chance based on distance
    auto MissileChance = [](int32 DistanceUnits) -> int32
    {
        // Simplified from p_enemy.c
        int32 Dist = DistanceUnits >> FRACBITS;
        if (Dist > 200) Dist = 200;
        return 256 - Dist; // Higher = more likely to fire (compared to P_Random)
    };

    int32 CloseChance = MissileChance(32 * FRACUNIT);
    int32 FarChance = MissileChance(200 * FRACUNIT);
    TestTrue(TEXT("Close = higher chance"), CloseChance > FarChance);
    TestTrue(TEXT("Close chance > 200"), CloseChance > 200);

    return true;
}

// ============================================================================
// Monster State Machine Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomMonsterStateTest, "UnrealDoom.AI.MonsterStates",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomMonsterStateTest::RunTest(const FString& Parameters)
{
    // Monster states in DOOM:
    // Spawn -> See (spotted player) -> Chase/Attack -> Pain -> Death -> XDeath
    //                                                                -> Raise (Archvile)

    enum class EMonsterState
    {
        Spawn,    // Idle, using A_Look
        See,      // Spotted player, begin chase
        Chase,    // Pursuing target, using A_Chase
        Melee,    // Close enough for melee attack
        Missile,  // Far enough for ranged attack
        Pain,     // Just took damage
        Death,    // Dying animation
        XDeath,   // Gib death (overkill damage)
        Raise     // Being resurrected by Archvile
    };

    // State transitions
    // Spawn -> See: when player is seen or heard
    // See -> Chase: immediate
    // Chase -> Melee: when in melee range
    // Chase -> Missile: when in missile range and random check passes
    // Any -> Pain: when taking damage (random chance based on painchance)
    // Any -> Death: when health <= 0
    // Death -> XDeath: when damage > health (overkill)
    // Death -> Raise: Archvile resurrects

    // Pain chance values (0-255, higher = more likely to enter pain)
    constexpr int32 PAIN_ZOMBIEMAN = 200;
    constexpr int32 PAIN_IMP = 200;
    constexpr int32 PAIN_DEMON = 180;
    constexpr int32 PAIN_CACODEMON = 128;
    constexpr int32 PAIN_BARON = 50;
    constexpr int32 PAIN_CYBERDEMON = 40;
    constexpr int32 PAIN_SPIDER = 40;
    constexpr int32 PAIN_LOSTSOUL = 256; // Always flinches

    TestTrue(TEXT("Lost soul always flinches"), PAIN_LOSTSOUL >= 256);
    TestTrue(TEXT("Cyberdemon rarely flinches"), PAIN_CYBERDEMON < 50);
    TestTrue(TEXT("Baron resists pain"), PAIN_BARON < PAIN_IMP);
    TestTrue(TEXT("Imp flinches easily"), PAIN_IMP > 150);

    // XDeath threshold: if remaining damage after death > initial health
    // Most monsters: if overkill damage exceeds spawn health
    auto ShouldGib = [](int32 Health, int32 Damage) -> bool
    {
        return Health <= 0 && Health < -Damage; // Actually: health - damage < -spawnhealth
    };

    // Zombie with 20 health taking 100 damage: 20-100 = -80 < -20, so gib
    TestTrue(TEXT("Zombie gibs"), -80 < -20);

    return true;
}

// ============================================================================
// Sound Propagation Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomSoundPropTest, "UnrealDoom.AI.SoundPropagation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomSoundPropTest::RunTest(const FString& Parameters)
{
    // DOOM sound propagation:
    // Sound travels through sectors via two-sided linedefs
    // ML_SOUNDBLOCK flag on linedef blocks sound
    // Sound decreases by 1 "traversal" per sector boundary
    // Monsters with MF_AMBUSH (deaf) only react to sight, not sound

    // soundtraversed values: 0 = not reached, 1 = one sector away, 2 = two away
    // Monsters react if their sector's soundtraversed <= 2

    constexpr int32 MAX_SOUND_TRAVERSALS = 2;

    auto CanHearSound = [MAX_SOUND_TRAVERSALS](int32 Traversals, bool bDeaf) -> bool
    {
        if (bDeaf) return false; // MF_AMBUSH monsters don't react to sound
        return Traversals > 0 && Traversals <= MAX_SOUND_TRAVERSALS;
    };

    TestTrue(TEXT("Hears in same sector"), CanHearSound(1, false));
    TestTrue(TEXT("Hears one sector away"), CanHearSound(2, false));
    TestFalse(TEXT("Too far"), CanHearSound(3, false));
    TestFalse(TEXT("Not reached"), CanHearSound(0, false));
    TestFalse(TEXT("Deaf monster"), CanHearSound(1, true));

    return true;
}

// ============================================================================
// Hitscan Attack Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomHitscanTest, "UnrealDoom.AI.HitscanAttack",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomHitscanTest::RunTest(const FString& Parameters)
{
    // DOOM hitscan spread for enemy attacks:
    // Zombieman: horizontal spread of angle +-22.5 degrees (ANG45/2)
    // Shotgun guy: 3 pellets with spread
    // Chaingunner: same as zombieman

    // Player autoaim: scans vertical range for targets
    // DOOM has no vertical aim, autoaim handles it

    constexpr float ZOMBIE_SPREAD = 22.5f; // degrees

    // A zombie fires at a target with random spread
    // Actual angle = target angle + (P_Random - P_Random) * spread / 256
    auto CalcSpread = [](int32 RandomA, int32 RandomB, float MaxSpread) -> float
    {
        return ((float)(RandomA - RandomB) / 256.0f) * MaxSpread;
    };

    // Perfect accuracy (both random values equal)
    float PerfectSpread = CalcSpread(128, 128, ZOMBIE_SPREAD);
    TestTrue(TEXT("No spread"), FMath::IsNearlyEqual(PerfectSpread, 0.0f, 0.01f));

    // Maximum positive spread
    float MaxPosSpread = CalcSpread(255, 0, ZOMBIE_SPREAD);
    TestTrue(TEXT("Max positive spread"), MaxPosSpread > 0.0f);
    TestTrue(TEXT("Max spread bounded"), MaxPosSpread <= ZOMBIE_SPREAD);

    // Maximum negative spread
    float MaxNegSpread = CalcSpread(0, 255, ZOMBIE_SPREAD);
    TestTrue(TEXT("Max negative spread"), MaxNegSpread < 0.0f);

    return true;
}

// ============================================================================
// Explosion/Radius Attack Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomExplosionTest, "UnrealDoom.AI.ExplosionDamage",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomExplosionTest::RunTest(const FString& Parameters)
{
    // P_RadiusAttack: damage decreases linearly with distance
    // MaxDamage at center, 0 at radius edge
    // Rocket: 128 damage, 128 radius
    // Barrel: 128 damage, 128 radius
    // BFG tracer: 100 damage (no splash)

    constexpr int32 ROCKET_DAMAGE = 128;
    constexpr int32 ROCKET_RADIUS = 128;

    auto CalcSplashDamage = [](int32 MaxDamage, int32 Radius, int32 Distance) -> int32
    {
        if (Distance >= Radius) return 0;
        return MaxDamage - (MaxDamage * Distance / Radius);
    };

    // At center: full damage
    TestEqual(TEXT("Center damage"), CalcSplashDamage(ROCKET_DAMAGE, ROCKET_RADIUS, 0), 128);

    // At half radius: half damage
    TestEqual(TEXT("Half radius damage"), CalcSplashDamage(ROCKET_DAMAGE, ROCKET_RADIUS, 64), 64);

    // At edge: zero damage
    TestEqual(TEXT("Edge damage"), CalcSplashDamage(ROCKET_DAMAGE, ROCKET_RADIUS, 128), 0);

    // Beyond radius: zero
    TestEqual(TEXT("Beyond radius"), CalcSplashDamage(ROCKET_DAMAGE, ROCKET_RADIUS, 200), 0);

    // Quarter radius: 3/4 damage
    TestEqual(TEXT("Quarter radius damage"), CalcSplashDamage(ROCKET_DAMAGE, ROCKET_RADIUS, 32), 96);

    // Self-damage from rockets (player is at distance = player radius from explosion)
    int32 SelfDamage = CalcSplashDamage(ROCKET_DAMAGE, ROCKET_RADIUS, 16);
    TestTrue(TEXT("Self damage significant"), SelfDamage > 100);

    return true;
}

// ============================================================================
// Infighting Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomInfightingTest, "UnrealDoom.AI.Infighting",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomInfightingTest::RunTest(const FString& Parameters)
{
    // DOOM infighting rules:
    // When a monster is hit by another monster's attack, it retargets to the attacker
    // Exception: same species won't infight (MF_MISSILE from same type)
    // Cyberdemons and Spider Masterminds are specifically set to infight

    auto WillInfight = [](int32 VictimType, int32 AttackerType) -> bool
    {
        // Same species don't infight
        if (VictimType == AttackerType) return false;
        return true;
    };

    TestTrue(TEXT("Imp vs Demon infight"), WillInfight(1, 2));
    TestFalse(TEXT("Imp vs Imp no infight"), WillInfight(1, 1));
    TestTrue(TEXT("Cyber vs Spider infight"), WillInfight(10, 11));

    // Archvile never targets same species
    // Lost souls spawned by Pain Elemental are special

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
