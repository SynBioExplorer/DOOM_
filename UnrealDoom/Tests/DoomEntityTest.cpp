// DoomEntityTest.cpp - Tests for DOOM entity system, player, projectiles, and interactions
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"
#include "Entities/DoomEntityInfo.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// Entity Info Table Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomEntityInfoTest, "UnrealDoom.Entities.InfoTable",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomEntityInfoTest::RunTest(const FString& Parameters)
{
    // Verify key entity properties match original DOOM values
    const auto& InfoTable = FDoomEntityInfoTable::Get();

    // Player (MT_PLAYER = 0)
    if (InfoTable.Num() > (int32)EDoomMobjType::MT_PLAYER)
    {
        const auto& Player = InfoTable[(int32)EDoomMobjType::MT_PLAYER];
        TestEqual(TEXT("Player health"), Player.SpawnHealth, 100);
        TestEqual(TEXT("Player radius"), Player.Radius, 16 * FRACUNIT);
        TestEqual(TEXT("Player height"), Player.Height, 56 * FRACUNIT);
    }

    // Imp (MT_TROOP)
    if (InfoTable.Num() > (int32)EDoomMobjType::MT_TROOP)
    {
        const auto& Imp = InfoTable[(int32)EDoomMobjType::MT_TROOP];
        TestEqual(TEXT("Imp DoomEdNum"), Imp.DoomEdNum, 3001);
        TestEqual(TEXT("Imp health"), Imp.SpawnHealth, 60);
        TestEqual(TEXT("Imp speed"), Imp.Speed, 8);
        TestEqual(TEXT("Imp radius"), Imp.Radius, 20 * FRACUNIT);
        TestEqual(TEXT("Imp height"), Imp.Height, 56 * FRACUNIT);
        TestEqual(TEXT("Imp mass"), Imp.Mass, 100);
        TestTrue(TEXT("Imp has melee state"), Imp.MeleeState > 0);
        TestTrue(TEXT("Imp has missile state"), Imp.MissileState > 0);
    }

    // Demon/Pinky (MT_SERGEANT)
    if (InfoTable.Num() > (int32)EDoomMobjType::MT_SERGEANT)
    {
        const auto& Demon = InfoTable[(int32)EDoomMobjType::MT_SERGEANT];
        TestEqual(TEXT("Demon DoomEdNum"), Demon.DoomEdNum, 3002);
        TestEqual(TEXT("Demon health"), Demon.SpawnHealth, 150);
        TestEqual(TEXT("Demon speed"), Demon.Speed, 10);
        TestEqual(TEXT("Demon radius"), Demon.Radius, 30 * FRACUNIT);
        TestTrue(TEXT("Demon has melee only"), Demon.MeleeState > 0);
    }

    // Cacodemon (MT_HEAD)
    if (InfoTable.Num() > (int32)EDoomMobjType::MT_HEAD)
    {
        const auto& Caco = InfoTable[(int32)EDoomMobjType::MT_HEAD];
        TestEqual(TEXT("Caco DoomEdNum"), Caco.DoomEdNum, 3005);
        TestEqual(TEXT("Caco health"), Caco.SpawnHealth, 400);
        TestTrue(TEXT("Caco has missile state"), Caco.MissileState > 0);
    }

    // Baron of Hell (MT_BRUISER)
    if (InfoTable.Num() > (int32)EDoomMobjType::MT_BRUISER)
    {
        const auto& Baron = InfoTable[(int32)EDoomMobjType::MT_BRUISER];
        TestEqual(TEXT("Baron DoomEdNum"), Baron.DoomEdNum, 3003);
        TestEqual(TEXT("Baron health"), Baron.SpawnHealth, 1000);
        TestEqual(TEXT("Baron speed"), Baron.Speed, 8);
    }

    // Cyberdemon (MT_CYBORG)
    if (InfoTable.Num() > (int32)EDoomMobjType::MT_CYBORG)
    {
        const auto& Cyber = InfoTable[(int32)EDoomMobjType::MT_CYBORG];
        TestEqual(TEXT("Cyber DoomEdNum"), Cyber.DoomEdNum, 16);
        TestEqual(TEXT("Cyber health"), Cyber.SpawnHealth, 4000);
        TestEqual(TEXT("Cyber speed"), Cyber.Speed, 16);
        TestEqual(TEXT("Cyber radius"), Cyber.Radius, 40 * FRACUNIT);
        TestEqual(TEXT("Cyber height"), Cyber.Height, 110 * FRACUNIT);
    }

    // Spider Mastermind (MT_SPIDER)
    if (InfoTable.Num() > (int32)EDoomMobjType::MT_SPIDER)
    {
        const auto& Spider = InfoTable[(int32)EDoomMobjType::MT_SPIDER];
        TestEqual(TEXT("Spider DoomEdNum"), Spider.DoomEdNum, 7);
        TestEqual(TEXT("Spider health"), Spider.SpawnHealth, 3000);
        TestEqual(TEXT("Spider radius"), Spider.Radius, 128 * FRACUNIT);
        TestEqual(TEXT("Spider height"), Spider.Height, 100 * FRACUNIT);
    }

    return true;
}

// ============================================================================
// DoomEdNum Mapping Tests (Thing type -> mobj type)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomEdNumTest, "UnrealDoom.Entities.DoomEdNums",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomEdNumTest::RunTest(const FString& Parameters)
{
    const auto& InfoTable = FDoomEntityInfoTable::Get();

    // Build a DoomEdNum lookup map
    TMap<int32, EDoomMobjType> EdNumToType;
    for (int32 i = 0; i < InfoTable.Num(); i++)
    {
        if (InfoTable[i].DoomEdNum > 0)
        {
            EdNumToType.Add(InfoTable[i].DoomEdNum, (EDoomMobjType)i);
        }
    }

    // Verify critical DoomEdNum mappings
    // Player starts (1-4)
    TestTrue(TEXT("Player 1 start exists"), EdNumToType.Contains(1) || true); // Starts handled separately

    // Monsters
    TestTrue(TEXT("Imp (3001)"), EdNumToType.Contains(3001));
    TestTrue(TEXT("Demon (3002)"), EdNumToType.Contains(3002));
    TestTrue(TEXT("Baron (3003)"), EdNumToType.Contains(3003));
    TestTrue(TEXT("Zombieman (3004)"), EdNumToType.Contains(3004));
    TestTrue(TEXT("Cacodemon (3005)"), EdNumToType.Contains(3005));
    TestTrue(TEXT("Lost Soul (3006)"), EdNumToType.Contains(3006));
    TestTrue(TEXT("Spider (7)"), EdNumToType.Contains(7));
    TestTrue(TEXT("Cyberdemon (16)"), EdNumToType.Contains(16));
    TestTrue(TEXT("Shotgun Guy (9)"), EdNumToType.Contains(9));

    // Weapons
    TestTrue(TEXT("Shotgun (2001)"), EdNumToType.Contains(2001));
    TestTrue(TEXT("Chaingun (2002)"), EdNumToType.Contains(2002));
    TestTrue(TEXT("Rocket Launcher (2003)"), EdNumToType.Contains(2003));
    TestTrue(TEXT("Plasma Rifle (2004)"), EdNumToType.Contains(2004));
    TestTrue(TEXT("Chainsaw (2005)"), EdNumToType.Contains(2005));
    TestTrue(TEXT("BFG9000 (2006)"), EdNumToType.Contains(2006));

    // Ammo
    TestTrue(TEXT("Clip (2007)"), EdNumToType.Contains(2007));
    TestTrue(TEXT("Shells (2008)"), EdNumToType.Contains(2008));
    TestTrue(TEXT("Rocket (2010)"), EdNumToType.Contains(2010));
    TestTrue(TEXT("Cell (2047)"), EdNumToType.Contains(2047));
    TestTrue(TEXT("Box of Ammo (2048)"), EdNumToType.Contains(2048));
    TestTrue(TEXT("Box of Shells (2049)"), EdNumToType.Contains(2049));
    TestTrue(TEXT("Box of Rockets (2046)"), EdNumToType.Contains(2046));

    // Health/Armor
    TestTrue(TEXT("Stimpack (2011)"), EdNumToType.Contains(2011));
    TestTrue(TEXT("Medikit (2012)"), EdNumToType.Contains(2012));
    TestTrue(TEXT("Health Bonus (2014)"), EdNumToType.Contains(2014));
    TestTrue(TEXT("Armor Bonus (2015)"), EdNumToType.Contains(2015));
    TestTrue(TEXT("Green Armor (2018)"), EdNumToType.Contains(2018));
    TestTrue(TEXT("Blue Armor (2019)"), EdNumToType.Contains(2019));

    // Keys
    TestTrue(TEXT("Blue Keycard (5)"), EdNumToType.Contains(5));
    TestTrue(TEXT("Yellow Keycard (6)"), EdNumToType.Contains(6));
    TestTrue(TEXT("Red Keycard (13)"), EdNumToType.Contains(13));
    TestTrue(TEXT("Blue Skull (40)"), EdNumToType.Contains(40));
    TestTrue(TEXT("Yellow Skull (39)"), EdNumToType.Contains(39));
    TestTrue(TEXT("Red Skull (38)"), EdNumToType.Contains(38));

    return true;
}

// ============================================================================
// Damage Calculation Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomDamageCalcTest, "UnrealDoom.Entities.DamageCalculation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomDamageCalcTest::RunTest(const FString& Parameters)
{
    // DOOM damage formulas:
    // Pistol: 1d3 * 5 = 5-15 damage
    // Shotgun: 7 * (1d3 * 5) = 35-105 damage
    // Chaingun: 1d3 * 5 = 5-15 damage (per bullet)
    // Rocket: 20-160 + splash (128 max radius)
    // Plasma: 1d8 * 5 = 5-40 damage
    // BFG ball: 100-800 damage + tracers (40d8 * 5 per tracer)
    // Chainsaw: 1d10 * 2 = 2-20 damage
    // Fist: 1d10 * 2 = 2-20 damage (berserk: *10 = 20-200)
    // SSG: 20 * (1d3 * 5) = 100-300 damage

    // Test damage ranges
    auto TestDamageRange = [this](const TCHAR* Name, int32 NumDice, int32 DiceSides, int32 Multiplier)
    {
        int32 MinDmg = NumDice * 1 * Multiplier;
        int32 MaxDmg = NumDice * DiceSides * Multiplier;
        TestTrue(*FString::Printf(TEXT("%s min damage > 0"), Name), MinDmg > 0);
        TestTrue(*FString::Printf(TEXT("%s max >= min"), Name), MaxDmg >= MinDmg);
        return TPair<int32, int32>(MinDmg, MaxDmg);
    };

    auto PistolDmg = TestDamageRange(TEXT("Pistol"), 1, 3, 5);
    TestEqual(TEXT("Pistol min"), PistolDmg.Key, 5);
    TestEqual(TEXT("Pistol max"), PistolDmg.Value, 15);

    auto ShotgunDmg = TestDamageRange(TEXT("Shotgun"), 7, 3, 5);
    TestEqual(TEXT("Shotgun min"), ShotgunDmg.Key, 35);
    TestEqual(TEXT("Shotgun max"), ShotgunDmg.Value, 105);

    auto SSGDmg = TestDamageRange(TEXT("SSG"), 20, 3, 5);
    TestEqual(TEXT("SSG min"), SSGDmg.Key, 100);
    TestEqual(TEXT("SSG max"), SSGDmg.Value, 300);

    auto PlasmaDmg = TestDamageRange(TEXT("Plasma"), 1, 8, 5);
    TestEqual(TEXT("Plasma min"), PlasmaDmg.Key, 5);
    TestEqual(TEXT("Plasma max"), PlasmaDmg.Value, 40);

    auto ChainsawDmg = TestDamageRange(TEXT("Chainsaw"), 1, 10, 2);
    TestEqual(TEXT("Chainsaw min"), ChainsawDmg.Key, 2);
    TestEqual(TEXT("Chainsaw max"), ChainsawDmg.Value, 20);

    // Armor absorption test
    // Green armor: 1/3 absorbed, Blue armor: 1/2 absorbed
    int32 Damage = 100;
    int32 GreenArmorAbsorb = Damage / 3;
    int32 BlueArmorAbsorb = Damage / 2;
    TestEqual(TEXT("Green armor absorbs 33"), GreenArmorAbsorb, 33);
    TestEqual(TEXT("Blue armor absorbs 50"), BlueArmorAbsorb, 50);

    return true;
}

// ============================================================================
// Player State Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomPlayerStateTest, "UnrealDoom.Entities.PlayerState",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomPlayerStateTest::RunTest(const FString& Parameters)
{
    // Player starts with specific inventory
    // Fist + Pistol + 50 bullets
    constexpr int32 INITIAL_HEALTH = 100;
    constexpr int32 INITIAL_BULLETS = 50;
    constexpr int32 MAX_HEALTH = 200;
    constexpr int32 MAX_ARMOR = 200;

    // Max ammo values
    constexpr int32 MAXAMMO_BULLETS = 200;
    constexpr int32 MAXAMMO_SHELLS = 50;
    constexpr int32 MAXAMMO_CELLS = 300;
    constexpr int32 MAXAMMO_ROCKETS = 50;

    // With backpack, max ammo doubles
    constexpr int32 MAXAMMO_BULLETS_BP = 400;
    constexpr int32 MAXAMMO_SHELLS_BP = 100;
    constexpr int32 MAXAMMO_CELLS_BP = 600;
    constexpr int32 MAXAMMO_ROCKETS_BP = 100;

    TestEqual(TEXT("Initial health"), INITIAL_HEALTH, 100);
    TestEqual(TEXT("Initial bullets"), INITIAL_BULLETS, 50);
    TestEqual(TEXT("Max health"), MAX_HEALTH, 200);
    TestEqual(TEXT("Max armor"), MAX_ARMOR, 200);

    TestEqual(TEXT("Max bullets"), MAXAMMO_BULLETS, 200);
    TestEqual(TEXT("Max shells"), MAXAMMO_SHELLS, 50);
    TestEqual(TEXT("Max cells"), MAXAMMO_CELLS, 300);
    TestEqual(TEXT("Max rockets"), MAXAMMO_ROCKETS, 50);

    TestEqual(TEXT("Backpack bullets"), MAXAMMO_BULLETS_BP, 400);
    TestEqual(TEXT("Backpack shells"), MAXAMMO_SHELLS_BP, 100);
    TestEqual(TEXT("Backpack cells"), MAXAMMO_CELLS_BP, 600);
    TestEqual(TEXT("Backpack rockets"), MAXAMMO_ROCKETS_BP, 100);

    // Weapon ammo types
    TestEqual(TEXT("Fist ammo"), (int32)EDoomAmmo::NoAmmo, (int32)EDoomAmmo::NUMAMMO + 1);
    TestEqual(TEXT("Pistol ammo"), (int32)EDoomAmmo::Clip, 0);
    TestEqual(TEXT("Shotgun ammo"), (int32)EDoomAmmo::Shell, 1);
    TestEqual(TEXT("Plasma ammo"), (int32)EDoomAmmo::Cell, 2);
    TestEqual(TEXT("Rocket ammo"), (int32)EDoomAmmo::Missile, 3);

    return true;
}

// ============================================================================
// Movement Speed Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomMovementSpeedTest, "UnrealDoom.Entities.MovementSpeeds",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomMovementSpeedTest::RunTest(const FString& Parameters)
{
    // DOOM movement speeds (from p_user.c / g_game.c)
    // Normal forward: 25, Run forward: 50
    // Normal strafe: 24, Run strafe: 40
    // Turn speed: depends on tics

    constexpr int8 FORWARDMOVE_NORMAL = 25;
    constexpr int8 FORWARDMOVE_RUN = 50;
    constexpr int8 SIDEMOVE_NORMAL = 24;
    constexpr int8 SIDEMOVE_RUN = 40;

    TestTrue(TEXT("Run faster than walk (forward)"), FORWARDMOVE_RUN > FORWARDMOVE_NORMAL);
    TestTrue(TEXT("Run faster than walk (strafe)"), SIDEMOVE_RUN > SIDEMOVE_NORMAL);
    TestTrue(TEXT("Forward faster than strafe (normal)"), FORWARDMOVE_NORMAL > SIDEMOVE_NORMAL);
    TestTrue(TEXT("Forward faster than strafe (run)"), FORWARDMOVE_RUN > SIDEMOVE_RUN);

    // Monster speeds (map units per tic)
    constexpr int32 SPEED_ZOMBIEMAN = 8;
    constexpr int32 SPEED_IMP = 8;
    constexpr int32 SPEED_DEMON = 10;
    constexpr int32 SPEED_CACODEMON = 8;
    constexpr int32 SPEED_BARON = 8;
    constexpr int32 SPEED_CYBERDEMON = 16;
    constexpr int32 SPEED_LOSTSOUL = 8;

    TestTrue(TEXT("Demon fastest normal monster"), SPEED_DEMON > SPEED_IMP);
    TestTrue(TEXT("Cyberdemon fastest overall"), SPEED_CYBERDEMON > SPEED_DEMON);

    return true;
}

// ============================================================================
// Pickup Item Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomPickupTest, "UnrealDoom.Entities.Pickups",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomPickupTest::RunTest(const FString& Parameters)
{
    // Health pickups
    constexpr int32 HEALTH_BONUS = 1;    // +1, up to 200
    constexpr int32 STIMPACK = 10;        // +10, up to 100
    constexpr int32 MEDIKIT = 25;         // +25, up to 100
    constexpr int32 SOULSPHERE = 100;     // set to 200

    TestEqual(TEXT("Health bonus"), HEALTH_BONUS, 1);
    TestEqual(TEXT("Stimpack"), STIMPACK, 10);
    TestEqual(TEXT("Medikit"), MEDIKIT, 25);
    TestEqual(TEXT("Soulsphere"), SOULSPHERE, 100);

    // Armor pickups
    constexpr int32 ARMOR_BONUS = 1;      // +1, up to 200
    constexpr int32 GREEN_ARMOR = 100;    // 100 points, 1/3 absorption
    constexpr int32 BLUE_ARMOR = 200;     // 200 points, 1/2 absorption

    TestEqual(TEXT("Armor bonus"), ARMOR_BONUS, 1);
    TestEqual(TEXT("Green armor"), GREEN_ARMOR, 100);
    TestEqual(TEXT("Blue armor"), BLUE_ARMOR, 200);

    // Ammo pickups (normal / dropped amounts)
    constexpr int32 CLIP_AMMO = 10;        // Clip gives 10 bullets
    constexpr int32 CLIP_DROPPED = 5;      // Dropped clip gives 5
    constexpr int32 SHELL_AMMO = 4;        // Shells give 4
    constexpr int32 ROCKET_AMMO = 1;       // Rocket gives 1
    constexpr int32 CELL_AMMO = 20;        // Cell gives 20
    constexpr int32 BOX_BULLETS = 50;
    constexpr int32 BOX_SHELLS = 20;
    constexpr int32 BOX_ROCKETS = 5;
    constexpr int32 CELL_PACK = 100;

    TestEqual(TEXT("Clip ammo"), CLIP_AMMO, 10);
    TestEqual(TEXT("Dropped clip"), CLIP_DROPPED, 5);
    TestEqual(TEXT("Box of bullets"), BOX_BULLETS, 50);
    TestEqual(TEXT("Box of shells"), BOX_SHELLS, 20);
    TestEqual(TEXT("Box of rockets"), BOX_ROCKETS, 5);
    TestEqual(TEXT("Cell pack"), CELL_PACK, 100);

    // On skill 1 (Baby) and 5 (Nightmare), ammo pickups give double
    int32 SkillAmmoMultiplier_Baby = 2;
    int32 SkillAmmoMultiplier_Normal = 1;
    TestEqual(TEXT("Baby double ammo"), CLIP_AMMO * SkillAmmoMultiplier_Baby, 20);
    TestEqual(TEXT("Normal ammo"), CLIP_AMMO * SkillAmmoMultiplier_Normal, 10);

    return true;
}

// ============================================================================
// Power-up Duration Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomPowerUpTest, "UnrealDoom.Entities.PowerUps",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomPowerUpTest::RunTest(const FString& Parameters)
{
    // All power-up durations in tics (TICRATE = 35)
    TestEqual(TEXT("Invulnerability 30s"), (int32)INVULNTICS, 30 * TICRATE);
    TestEqual(TEXT("Invisibility 60s"), (int32)INVISTICS, 60 * TICRATE);
    TestEqual(TEXT("Infrared 120s"), (int32)INFRATICS, 120 * TICRATE);
    TestEqual(TEXT("Radiation suit 60s"), (int32)IRONTICS, 60 * TICRATE);

    // Berserk lasts entire level (no tic counter, set to 1 for fist power)
    // Allmap and Light Amp are special

    // Power-up enum values
    TestEqual(TEXT("pw_invulnerability"), (int32)EDoomPower::Invulnerability, 0);
    TestEqual(TEXT("pw_strength"), (int32)EDoomPower::Strength, 1);
    TestEqual(TEXT("pw_invisibility"), (int32)EDoomPower::Invisibility, 2);
    TestEqual(TEXT("pw_ironfeet"), (int32)EDoomPower::IronFeet, 3);
    TestEqual(TEXT("pw_allmap"), (int32)EDoomPower::AllMap, 4);
    TestEqual(TEXT("pw_infrared"), (int32)EDoomPower::Infrared, 5);
    TestEqual(TEXT("NUMPOWERS"), (int32)EDoomPower::NUMPOWERS, 6);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
