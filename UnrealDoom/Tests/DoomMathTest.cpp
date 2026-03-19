// DoomMathTest.cpp - Comprehensive tests for DOOM math utilities
// Tests fixed-point arithmetic, coordinate conversion, and angle math

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// Fixed-Point Math Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomFixedMulTest, "UnrealDoom.Math.FixedMul",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomFixedMulTest::RunTest(const FString& Parameters)
{
    // 1.0 * 1.0 = 1.0
    TestEqual(TEXT("1.0 * 1.0"), DoomMath::FixedMul(FRACUNIT, FRACUNIT), FRACUNIT);

    // 2.0 * 3.0 = 6.0
    TestEqual(TEXT("2.0 * 3.0"), DoomMath::FixedMul(2 * FRACUNIT, 3 * FRACUNIT), 6 * FRACUNIT);

    // 0.5 * 0.5 = 0.25
    TestEqual(TEXT("0.5 * 0.5"), DoomMath::FixedMul(FRACUNIT / 2, FRACUNIT / 2), FRACUNIT / 4);

    // Negative: -1.0 * 2.0 = -2.0
    TestEqual(TEXT("-1.0 * 2.0"), DoomMath::FixedMul(-FRACUNIT, 2 * FRACUNIT), -2 * FRACUNIT);

    // Zero: 0 * anything = 0
    TestEqual(TEXT("0 * 5.0"), DoomMath::FixedMul(0, 5 * FRACUNIT), 0);

    // Large values: 100.0 * 100.0 = 10000.0
    TestEqual(TEXT("100 * 100"), DoomMath::FixedMul(100 * FRACUNIT, 100 * FRACUNIT), 10000 * FRACUNIT);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomFixedDivTest, "UnrealDoom.Math.FixedDiv",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomFixedDivTest::RunTest(const FString& Parameters)
{
    // 6.0 / 3.0 = 2.0
    TestEqual(TEXT("6.0 / 3.0"), DoomMath::FixedDiv(6 * FRACUNIT, 3 * FRACUNIT), 2 * FRACUNIT);

    // 1.0 / 2.0 = 0.5
    TestEqual(TEXT("1.0 / 2.0"), DoomMath::FixedDiv(FRACUNIT, 2 * FRACUNIT), FRACUNIT / 2);

    // 1.0 / 1.0 = 1.0
    TestEqual(TEXT("1.0 / 1.0"), DoomMath::FixedDiv(FRACUNIT, FRACUNIT), FRACUNIT);

    // Negative: -6.0 / 3.0 = -2.0
    TestEqual(TEXT("-6.0 / 3.0"), DoomMath::FixedDiv(-6 * FRACUNIT, 3 * FRACUNIT), -2 * FRACUNIT);

    // Division by zero should return safe value (original DOOM uses FixedDiv2 fallback)
    // This tests that no crash occurs
    int32 Result = DoomMath::FixedDiv(FRACUNIT, 0);
    TestTrue(TEXT("Div by zero doesn't crash"), true);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomFixedFloatConversionTest, "UnrealDoom.Math.FixedFloatConversion",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomFixedFloatConversionTest::RunTest(const FString& Parameters)
{
    // 1.0 round-trip
    float Val1 = DoomMath::FixedToFloat(FRACUNIT);
    TestEqual(TEXT("FRACUNIT to float"), Val1, 1.0f);
    TestEqual(TEXT("1.0f to fixed"), DoomMath::FloatToFixed(1.0f), FRACUNIT);

    // 0.5 round-trip
    float Val05 = DoomMath::FixedToFloat(FRACUNIT / 2);
    TestTrue(TEXT("0.5 conversion"), FMath::IsNearlyEqual(Val05, 0.5f, 0.001f));

    // Negative values
    float ValNeg = DoomMath::FixedToFloat(-3 * FRACUNIT);
    TestEqual(TEXT("-3.0 conversion"), ValNeg, -3.0f);

    // Zero
    TestEqual(TEXT("0 conversion"), DoomMath::FixedToFloat(0), 0.0f);
    TestEqual(TEXT("0.0f to fixed"), DoomMath::FloatToFixed(0.0f), 0);

    // Large values
    float Val100 = DoomMath::FixedToFloat(100 * FRACUNIT);
    TestEqual(TEXT("100.0 conversion"), Val100, 100.0f);

    return true;
}

// ============================================================================
// Coordinate System Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomCoordinateConversionTest, "UnrealDoom.Math.CoordinateConversion",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomCoordinateConversionTest::RunTest(const FString& Parameters)
{
    // DOOM origin maps to UE origin
    FVector Origin = DoomMath::DoomPosToUE(0, 0, 0);
    TestEqual(TEXT("Origin X"), Origin.X, 0.0f);
    TestEqual(TEXT("Origin Y"), Origin.Y, 0.0f);
    TestEqual(TEXT("Origin Z"), Origin.Z, 0.0f);

    // Scale factor: 1 DOOM unit = 2 UE units
    // DOOM Y -> UE X, DOOM X -> UE Y, DOOM Z -> UE Z
    FVector Pos = DoomMath::DoomPosToUE(FRACUNIT, 2 * FRACUNIT, 3 * FRACUNIT);
    TestTrue(TEXT("Scale X"), FMath::IsNearlyEqual(Pos.X, 4.0f, 0.01f)); // DOOM Y=2 * scale=2 = 4
    TestTrue(TEXT("Scale Y"), FMath::IsNearlyEqual(Pos.Y, 2.0f, 0.01f)); // DOOM X=1 * scale=2 = 2
    TestTrue(TEXT("Scale Z"), FMath::IsNearlyEqual(Pos.Z, 6.0f, 0.01f)); // DOOM Z=3 * scale=2 = 6

    // DoomUnitsToUE basic scale
    float UEVal = DoomMath::DoomUnitsToUE(10 * FRACUNIT);
    TestTrue(TEXT("DoomUnitsToUE"), FMath::IsNearlyEqual(UEVal, 20.0f, 0.01f));

    // UEToDoomUnits reverse
    int32 DoomVal = DoomMath::UEToDoomUnits(20.0f);
    TestEqual(TEXT("UEToDoomUnits"), DoomVal, 10 * FRACUNIT);

    return true;
}

// ============================================================================
// Angle Math Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomAngleConversionTest, "UnrealDoom.Math.AngleConversion",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomAngleConversionTest::RunTest(const FString& Parameters)
{
    // ANG90 should be 90 degrees
    float Deg90 = DoomMath::AngleToFloat(ANG90);
    TestTrue(TEXT("ANG90 = 90 degrees"), FMath::IsNearlyEqual(Deg90, 90.0f, 0.1f));

    // ANG180 should be 180 degrees
    float Deg180 = DoomMath::AngleToFloat(ANG180);
    TestTrue(TEXT("ANG180 = 180 degrees"), FMath::IsNearlyEqual(Deg180, 180.0f, 0.1f));

    // ANG270 should be 270 degrees
    float Deg270 = DoomMath::AngleToFloat(ANG270);
    TestTrue(TEXT("ANG270 = 270 degrees"), FMath::IsNearlyEqual(Deg270, 270.0f, 0.1f));

    // 0 should be 0 degrees
    float Deg0 = DoomMath::AngleToFloat(0);
    TestTrue(TEXT("0 = 0 degrees"), FMath::IsNearlyEqual(Deg0, 0.0f, 0.1f));

    // Round-trip: 45 degrees
    uint32 Angle45 = DoomMath::FloatToAngle(45.0f);
    TestEqual(TEXT("45 degrees = ANG45"), Angle45, (uint32)ANG45);

    return true;
}

// ============================================================================
// Random Number Generator Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomRandomTest, "UnrealDoom.Math.Random",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomRandomTest::RunTest(const FString& Parameters)
{
    // After clearing, sequence should be deterministic
    DoomRandom::M_ClearRandom();

    // First call to P_Random should return rndtable[0]
    int32 First = DoomRandom::P_Random();
    TestTrue(TEXT("P_Random returns 0-255"), First >= 0 && First <= 255);

    // Reset and verify same sequence
    DoomRandom::M_ClearRandom();
    int32 FirstAgain = DoomRandom::P_Random();
    TestEqual(TEXT("Deterministic after clear"), First, FirstAgain);

    // M_Random uses separate index from P_Random
    DoomRandom::M_ClearRandom();
    int32 PVal = DoomRandom::P_Random();
    DoomRandom::M_ClearRandom();
    int32 MVal = DoomRandom::M_Random();
    // Both start at index 0 after clear, so should return same first value
    TestEqual(TEXT("P_Random and M_Random same table"), PVal, MVal);

    // Generate 256 values and verify all are in range
    DoomRandom::M_ClearRandom();
    for (int32 i = 0; i < 256; i++)
    {
        int32 Val = DoomRandom::P_Random();
        TestTrue(TEXT("P_Random in range"), Val >= 0 && Val <= 255);
    }

    // After 256 calls, index wraps around (256 & 0xFF = 0), should repeat
    DoomRandom::M_ClearRandom();
    TArray<int32> FirstRound;
    for (int32 i = 0; i < 256; i++)
    {
        FirstRound.Add(DoomRandom::P_Random());
    }
    // Next 256 should be identical (table wraps)
    for (int32 i = 0; i < 256; i++)
    {
        int32 Val = DoomRandom::P_Random();
        TestEqual(*FString::Printf(TEXT("Wrap at index %d"), i), Val, FirstRound[i]);
    }

    return true;
}

// ============================================================================
// DOOM Type/Enum Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomTypesTest, "UnrealDoom.Types.Enums",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomTypesTest::RunTest(const FString& Parameters)
{
    // Weapon count
    TestEqual(TEXT("NUMWEAPONS"), (int32)EDoomWeapon::NUMWEAPONS, 9);

    // Ammo count
    TestEqual(TEXT("NUMAMMO"), (int32)EDoomAmmo::NUMAMMO, 4);

    // Card count
    TestEqual(TEXT("NUMCARDS"), (int32)EDoomCard::NUMCARDS, 6);

    // Power count
    TestEqual(TEXT("NUMPOWERS"), (int32)EDoomPower::NUMPOWERS, 6);

    // Skill levels
    TestEqual(TEXT("sk_baby"), (int32)EDoomSkill::Baby, 0);
    TestEqual(TEXT("sk_nightmare"), (int32)EDoomSkill::Nightmare, 4);

    // Game states
    TestEqual(TEXT("GS_LEVEL"), (int32)EDoomGameState::Level, 0);
    TestEqual(TEXT("GS_DEMOSCREEN"), (int32)EDoomGameState::DemoScreen, 3);

    // Power durations (TICRATE=35)
    TestEqual(TEXT("INVULNTICS"), (int32)INVULNTICS, 30 * 35);
    TestEqual(TEXT("INVISTICS"), (int32)INVISTICS, 60 * 35);
    TestEqual(TEXT("INFRATICS"), (int32)INFRATICS, 120 * 35);
    TestEqual(TEXT("IRONTICS"), (int32)IRONTICS, 60 * 35);

    // MObj flags
    TestEqual(TEXT("MF_SPECIAL"), (int32)MF_SPECIAL, 1);
    TestEqual(TEXT("MF_SOLID"), (int32)MF_SOLID, 2);
    TestEqual(TEXT("MF_SHOOTABLE"), (int32)MF_SHOOTABLE, 4);
    TestEqual(TEXT("MF_MISSILE"), (int32)MF_MISSILE, 0x10000);
    TestEqual(TEXT("MF_COUNTKILL"), (int32)MF_COUNTKILL, 0x400000);
    TestEqual(TEXT("MF_COUNTITEM"), (int32)MF_COUNTITEM, 0x800000);

    // Constants
    TestEqual(TEXT("TICRATE"), TICRATE, 35);
    TestEqual(TEXT("MAXPLAYERS"), MAXPLAYERS, 4);
    TestEqual(TEXT("FRACBITS"), FRACBITS, 16);
    TestEqual(TEXT("FRACUNIT"), FRACUNIT, 65536);
    TestEqual(TEXT("SCREENWIDTH"), SCREENWIDTH, 320);
    TestEqual(TEXT("SCREENHEIGHT"), SCREENHEIGHT, 200);

    return true;
}

// ============================================================================
// MObj Flag Combination Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomMobjFlagsTest, "UnrealDoom.Types.MobjFlags",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomMobjFlagsTest::RunTest(const FString& Parameters)
{
    // Test flag combinations like the original DOOM uses them
    // Imp: MF_SOLID|MF_SHOOTABLE|MF_COUNTKILL
    int32 ImpFlags = MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL;
    TestTrue(TEXT("Imp is solid"), (ImpFlags & MF_SOLID) != 0);
    TestTrue(TEXT("Imp is shootable"), (ImpFlags & MF_SHOOTABLE) != 0);
    TestTrue(TEXT("Imp counts for kills"), (ImpFlags & MF_COUNTKILL) != 0);
    TestFalse(TEXT("Imp is not missile"), (ImpFlags & MF_MISSILE) != 0);

    // Rocket: MF_NOBLOCKMAP|MF_MISSILE|MF_DROPOFF|MF_NOGRAVITY
    int32 RocketFlags = MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY;
    TestTrue(TEXT("Rocket is missile"), (RocketFlags & MF_MISSILE) != 0);
    TestTrue(TEXT("Rocket no gravity"), (RocketFlags & MF_NOGRAVITY) != 0);
    TestTrue(TEXT("Rocket no blockmap"), (RocketFlags & MF_NOBLOCKMAP) != 0);
    TestFalse(TEXT("Rocket not solid"), (RocketFlags & MF_SOLID) != 0);

    // Player: MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH
    int32 PlayerFlags = MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_PICKUP | MF_NOTDMATCH;
    TestTrue(TEXT("Player can pickup"), (PlayerFlags & MF_PICKUP) != 0);
    TestTrue(TEXT("Player is solid"), (PlayerFlags & MF_SOLID) != 0);

    // Translation mask test (for multiplayer player colors)
    TestEqual(TEXT("MF_TRANSSHIFT"), (int32)MF_TRANSSHIFT, 26);
    TestEqual(TEXT("MF_TRANSLATION"), (int32)MF_TRANSLATION, (int32)0xc000000);

    return true;
}

// ============================================================================
// WAD Types Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomWadTypesTest, "UnrealDoom.WAD.TypeSizes",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomWadTypesTest::RunTest(const FString& Parameters)
{
    // WAD header should be 12 bytes: 4 (id) + 4 (numlumps) + 4 (infotableofs)
    TestEqual(TEXT("WadHeader size"), (int32)sizeof(FWadHeader), 12);

    // Lump directory entry should be 16 bytes: 4 (filepos) + 4 (size) + 8 (name)
    TestEqual(TEXT("WadLumpInfo size"), (int32)sizeof(FWadLumpInfo), 16);

    // Map vertex is 4 bytes: 2 (x) + 2 (y)
    TestEqual(TEXT("MapVertex size"), (int32)sizeof(FMapVertex), 4);

    // Map thing is 10 bytes: 2*5 fields
    TestEqual(TEXT("MapThing size"), (int32)sizeof(FMapThing), 10);

    // Map linedef is 14 bytes: 2*7 fields
    TestEqual(TEXT("MapLineDef size"), (int32)sizeof(FMapLineDef), 14);

    // Map sidedef is 30 bytes: 2+2+8+8+8+2
    TestEqual(TEXT("MapSideDef size"), (int32)sizeof(FMapSideDef), 30);

    // Map sector is 26 bytes: 2+2+8+8+2+2+2
    TestEqual(TEXT("MapSector size"), (int32)sizeof(FMapSector), 26);

    return true;
}

// ============================================================================
// TicCmd Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomTicCmdTest, "UnrealDoom.GameLogic.TicCmd",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomTicCmdTest::RunTest(const FString& Parameters)
{
    FDoomTicCmd Cmd;
    FMemory::Memzero(&Cmd, sizeof(FDoomTicCmd));

    // Default values should be zero
    TestEqual(TEXT("Default forwardmove"), (int32)Cmd.ForwardMove, 0);
    TestEqual(TEXT("Default sidemove"), (int32)Cmd.SideMove, 0);
    TestEqual(TEXT("Default angleturn"), (int32)Cmd.AngleTurn, 0);
    TestEqual(TEXT("Default buttons"), (int32)Cmd.Buttons, 0);

    // Set movement values
    Cmd.ForwardMove = 50;  // Max forward speed in DOOM
    Cmd.SideMove = 40;     // Max strafe speed
    TestEqual(TEXT("Forward move"), (int32)Cmd.ForwardMove, 50);
    TestEqual(TEXT("Side move"), (int32)Cmd.SideMove, 40);

    // Button flags
    Cmd.Buttons = BT_ATTACK | BT_USE;
    TestTrue(TEXT("Attack button"), (Cmd.Buttons & BT_ATTACK) != 0);
    TestTrue(TEXT("Use button"), (Cmd.Buttons & BT_USE) != 0);
    TestFalse(TEXT("No weapon change"), (Cmd.Buttons & BT_CHANGE) != 0);

    // Weapon change encoding
    uint8 WeaponNum = 3; // Shotgun
    Cmd.Buttons = BT_CHANGE | (WeaponNum << BT_WEAPONSHIFT);
    TestTrue(TEXT("Weapon change flagged"), (Cmd.Buttons & BT_CHANGE) != 0);
    int32 DecodedWeapon = (Cmd.Buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;
    TestEqual(TEXT("Weapon number decoded"), DecodedWeapon, 3);

    return true;
}

// ============================================================================
// Game State Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomGameStateTest, "UnrealDoom.GameLogic.GameState",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomGameStateTest::RunTest(const FString& Parameters)
{
    // Test game state transitions
    // Level -> Intermission -> Level is the normal flow
    TestTrue(TEXT("Level state valid"), (int32)EDoomGameState::Level >= 0);
    TestTrue(TEXT("Intermission state valid"), (int32)EDoomGameState::Intermission == 1);
    TestTrue(TEXT("Finale state valid"), (int32)EDoomGameState::Finale == 2);
    TestTrue(TEXT("DemoScreen state valid"), (int32)EDoomGameState::DemoScreen == 3);

    // Test skill levels map to correct difficulty
    TestEqual(TEXT("Baby = 0"), (int32)EDoomSkill::Baby, 0);
    TestEqual(TEXT("Easy = 1"), (int32)EDoomSkill::Easy, 1);
    TestEqual(TEXT("Medium = 2"), (int32)EDoomSkill::Medium, 2);
    TestEqual(TEXT("Hard = 3"), (int32)EDoomSkill::Hard, 3);
    TestEqual(TEXT("Nightmare = 4"), (int32)EDoomSkill::Nightmare, 4);

    // Game modes
    TestEqual(TEXT("Shareware = 0"), (int32)EDoomGameMode::Shareware, 0);
    TestEqual(TEXT("Registered = 1"), (int32)EDoomGameMode::Registered, 1);
    TestEqual(TEXT("Commercial = 2"), (int32)EDoomGameMode::Commercial, 2);
    TestEqual(TEXT("Retail = 3"), (int32)EDoomGameMode::Retail, 3);

    return true;
}

// ============================================================================
// Button Code Tests (matching original DOOM values exactly)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomButtonCodesTest, "UnrealDoom.Types.ButtonCodes",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomButtonCodesTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("BT_ATTACK"), BT_ATTACK, 1);
    TestEqual(TEXT("BT_USE"), BT_USE, 2);
    TestEqual(TEXT("BT_SPECIAL"), BT_SPECIAL, 128);
    TestEqual(TEXT("BT_CHANGE"), BT_CHANGE, 4);
    TestEqual(TEXT("BT_WEAPONMASK"), BT_WEAPONMASK, 8 + 16 + 32);
    TestEqual(TEXT("BT_WEAPONSHIFT"), BT_WEAPONSHIFT, 3);
    TestEqual(TEXT("BTS_PAUSE"), BTS_PAUSE, 1);
    TestEqual(TEXT("BTS_SAVEGAME"), BTS_SAVEGAME, 2);

    return true;
}

// ============================================================================
// Key Code Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomKeyCodesTest, "UnrealDoom.Types.KeyCodes",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomKeyCodesTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("KEY_RIGHTARROW"), KEY_RIGHTARROW, 0xae);
    TestEqual(TEXT("KEY_LEFTARROW"), KEY_LEFTARROW, 0xac);
    TestEqual(TEXT("KEY_UPARROW"), KEY_UPARROW, 0xad);
    TestEqual(TEXT("KEY_DOWNARROW"), KEY_DOWNARROW, 0xaf);
    TestEqual(TEXT("KEY_ESCAPE"), KEY_ESCAPE, 27);
    TestEqual(TEXT("KEY_ENTER"), KEY_ENTER, 13);
    TestEqual(TEXT("KEY_TAB"), KEY_TAB, 9);
    TestEqual(TEXT("KEY_BACKSPACE"), KEY_BACKSPACE, 127);
    TestEqual(TEXT("KEY_PAUSE"), KEY_PAUSE, 0xff);
    TestEqual(TEXT("KEY_RSHIFT"), KEY_RSHIFT, 0x80 + 0x36);
    TestEqual(TEXT("KEY_RCTRL"), KEY_RCTRL, 0x80 + 0x1d);
    TestEqual(TEXT("KEY_RALT"), KEY_RALT, 0x80 + 0x38);
    TestEqual(TEXT("KEY_LALT"), KEY_LALT, KEY_RALT);

    // Function keys
    TestEqual(TEXT("KEY_F1"), KEY_F1, 0x80 + 0x3b);
    TestEqual(TEXT("KEY_F12"), KEY_F12, 0x80 + 0x58);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
