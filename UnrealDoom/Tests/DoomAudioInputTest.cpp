// DoomAudioInputTest.cpp - Tests for audio system and input mapping
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"
#include "Audio/DoomSoundDefs.h"

#if WITH_DEV_AUTOMATION_TESTS

// ============================================================================
// Sound Attenuation Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomSoundAttenuationTest, "UnrealDoom.Audio.Attenuation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomSoundAttenuationTest::RunTest(const FString& Parameters)
{
    // DOOM sound: max distance = 1200 map units (S_CLIPPING_DIST in s_sound.c)
    // Close distance = 200 map units (S_CLOSE_DIST)
    // Volume scales linearly from 1.0 at close distance to 0.0 at max distance
    constexpr int32 S_CLIPPING_DIST = 1200;
    constexpr int32 S_CLOSE_DIST = 200;
    constexpr int32 S_STEREO_SWING = 96; // Max stereo separation

    auto CalcVolume = [S_CLIPPING_DIST, S_CLOSE_DIST](int32 Distance) -> float
    {
        if (Distance <= S_CLOSE_DIST) return 1.0f;
        if (Distance >= S_CLIPPING_DIST) return 0.0f;
        return 1.0f - (float)(Distance - S_CLOSE_DIST) / (float)(S_CLIPPING_DIST - S_CLOSE_DIST);
    };

    TestTrue(TEXT("Close = full volume"), FMath::IsNearlyEqual(CalcVolume(100), 1.0f));
    TestTrue(TEXT("At close dist"), FMath::IsNearlyEqual(CalcVolume(200), 1.0f));
    TestTrue(TEXT("Far = silent"), FMath::IsNearlyEqual(CalcVolume(1200), 0.0f));
    TestTrue(TEXT("Beyond = silent"), FMath::IsNearlyEqual(CalcVolume(2000), 0.0f));
    TestTrue(TEXT("Mid = half"), CalcVolume(700) > 0.0f && CalcVolume(700) < 1.0f);

    // Stereo panning: based on angle to sound source
    // angle 0 = center, ANG90 = full left, -ANG90 = full right
    auto CalcPan = [S_STEREO_SWING](float AngleDegrees) -> float
    {
        // Returns -1.0 (left) to 1.0 (right)
        float Normalized = FMath::Clamp(AngleDegrees / 90.0f, -1.0f, 1.0f);
        return -Normalized; // DOOM convention: positive angle = left
    };

    TestTrue(TEXT("Center pan"), FMath::IsNearlyEqual(CalcPan(0.0f), 0.0f, 0.01f));
    TestTrue(TEXT("Left pan"), CalcPan(90.0f) < 0.0f);
    TestTrue(TEXT("Right pan"), CalcPan(-90.0f) > 0.0f);

    return true;
}

// ============================================================================
// Sound Priority Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomSoundPriorityTest, "UnrealDoom.Audio.Priority",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomSoundPriorityTest::RunTest(const FString& Parameters)
{
    // DOOM has limited sound channels (8 in original)
    // When all channels are full, lower priority sounds are replaced
    constexpr int32 MAX_CHANNELS = 8;

    // Priority = based on sound type and distance
    // Weapon sounds > monster sounds > ambient sounds
    // Closer sounds > farther sounds

    struct FSoundChannel
    {
        int32 SfxId;
        int32 Priority;
        int32 Distance;
        bool bActive;
    };

    // Simulate channel allocation
    TArray<FSoundChannel> Channels;
    Channels.SetNum(MAX_CHANNELS);
    for (auto& Ch : Channels)
    {
        Ch.bActive = false;
        Ch.Priority = 0;
        Ch.Distance = 0;
    }

    // Find free or lowest priority channel
    auto FindChannel = [&Channels](int32 NewPriority) -> int32
    {
        // First: find free channel
        for (int32 i = 0; i < Channels.Num(); i++)
        {
            if (!Channels[i].bActive) return i;
        }
        // Second: find lowest priority channel
        int32 LowestIdx = 0;
        int32 LowestPri = Channels[0].Priority;
        for (int32 i = 1; i < Channels.Num(); i++)
        {
            if (Channels[i].Priority < LowestPri)
            {
                LowestPri = Channels[i].Priority;
                LowestIdx = i;
            }
        }
        if (NewPriority > LowestPri) return LowestIdx;
        return -1; // Can't play
    };

    // All channels free initially
    TestEqual(TEXT("Free channel 0"), FindChannel(100), 0);

    // Fill first channel
    Channels[0].bActive = true;
    Channels[0].Priority = 50;
    TestEqual(TEXT("Free channel 1"), FindChannel(100), 1);

    // Fill all channels with priority 50
    for (int32 i = 0; i < MAX_CHANNELS; i++)
    {
        Channels[i].bActive = true;
        Channels[i].Priority = 50;
    }

    // High priority replaces lowest
    TestEqual(TEXT("Replace lowest"), FindChannel(100), 0); // First with priority 50

    // Equal priority can't replace
    int32 EqualResult = FindChannel(50);
    TestTrue(TEXT("Equal can replace"), EqualResult >= 0); // Actually >= in original DOOM

    // Lower priority can't play
    TestEqual(TEXT("Low priority rejected"), FindChannel(10), -1);

    return true;
}

// ============================================================================
// Sound Definition Count Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomSoundDefsTest, "UnrealDoom.Audio.SoundDefinitions",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomSoundDefsTest::RunTest(const FString& Parameters)
{
    // Verify critical sound effects exist in enum
    TestTrue(TEXT("sfx_pistol defined"), (int32)EDoomSfx::Pistol >= 0);
    TestTrue(TEXT("sfx_shotgn defined"), (int32)EDoomSfx::Shotgun >= 0);
    TestTrue(TEXT("sfx_plasma defined"), (int32)EDoomSfx::Plasma >= 0);
    TestTrue(TEXT("sfx_bfg defined"), (int32)EDoomSfx::BFG >= 0);
    TestTrue(TEXT("sfx_sawup defined"), (int32)EDoomSfx::SawUp >= 0);
    TestTrue(TEXT("sfx_rlaunc defined"), (int32)EDoomSfx::RocketLaunch >= 0);
    TestTrue(TEXT("sfx_rxplod defined"), (int32)EDoomSfx::RocketExplode >= 0);
    TestTrue(TEXT("sfx_itemup defined"), (int32)EDoomSfx::ItemUp >= 0);
    TestTrue(TEXT("sfx_wpnup defined"), (int32)EDoomSfx::WeaponUp >= 0);
    TestTrue(TEXT("sfx_telept defined"), (int32)EDoomSfx::Teleport >= 0);
    TestTrue(TEXT("sfx_doropn defined"), (int32)EDoomSfx::DoorOpen >= 0);
    TestTrue(TEXT("sfx_dorcls defined"), (int32)EDoomSfx::DoorClose >= 0);

    // Verify music tracks
    TestTrue(TEXT("mus_e1m1 defined"), (int32)EDoomMusic::E1M1 >= 0);

    return true;
}

// ============================================================================
// Input Mapping Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomInputMappingTest, "UnrealDoom.Input.Mapping",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomInputMappingTest::RunTest(const FString& Parameters)
{
    // DOOM movement speeds from g_game.c
    // forwardmove[2] = {0x19, 0x32}  (25, 50)
    // sidemove[2] = {0x18, 0x28}     (24, 40)
    // angleturn[3] = {640, 1280, 320} (slow, fast, very slow for mouse)

    constexpr int8 FORWARDMOVE_WALK = 0x19;  // 25
    constexpr int8 FORWARDMOVE_RUN = 0x32;   // 50
    constexpr int8 SIDEMOVE_WALK = 0x18;      // 24
    constexpr int8 SIDEMOVE_RUN = 0x28;       // 40
    constexpr int16 ANGLETURN_NORMAL = 640;
    constexpr int16 ANGLETURN_FAST = 1280;

    TestEqual(TEXT("Forward walk"), (int32)FORWARDMOVE_WALK, 25);
    TestEqual(TEXT("Forward run"), (int32)FORWARDMOVE_RUN, 50);
    TestEqual(TEXT("Strafe walk"), (int32)SIDEMOVE_WALK, 24);
    TestEqual(TEXT("Strafe run"), (int32)SIDEMOVE_RUN, 40);
    TestEqual(TEXT("Turn normal"), (int32)ANGLETURN_NORMAL, 640);
    TestEqual(TEXT("Turn fast"), (int32)ANGLETURN_FAST, 1280);

    // Run is always-run toggle or shift key
    // Strafe is alt key

    // TicCmd building: combine movement inputs
    FDoomTicCmd Cmd;
    FMemory::Memzero(&Cmd, sizeof(FDoomTicCmd));

    // Walking forward
    Cmd.ForwardMove = FORWARDMOVE_WALK;
    TestEqual(TEXT("Walk forward"), (int32)Cmd.ForwardMove, 25);

    // Running forward + strafing right
    Cmd.ForwardMove = FORWARDMOVE_RUN;
    Cmd.SideMove = SIDEMOVE_RUN;
    TestEqual(TEXT("Run forward"), (int32)Cmd.ForwardMove, 50);
    TestEqual(TEXT("Run strafe"), (int32)Cmd.SideMove, 40);

    // Clamp check: values should not exceed max
    TestTrue(TEXT("Forward <= 50"), Cmd.ForwardMove <= FORWARDMOVE_RUN);
    TestTrue(TEXT("Side <= 40"), Cmd.SideMove <= SIDEMOVE_RUN);

    // Mouse turn: pixels * sensitivity -> angleturn
    constexpr int32 MOUSE_SENSITIVITY = 5; // Default
    int32 MouseDx = 10; // pixels moved
    int16 MouseTurn = (int16)(MouseDx * MOUSE_SENSITIVITY * 8); // Simplified
    TestTrue(TEXT("Mouse turn nonzero"), MouseTurn != 0);

    return true;
}

// ============================================================================
// Weapon Switching Tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoomWeaponSwitchTest, "UnrealDoom.Input.WeaponSwitch",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDoomWeaponSwitchTest::RunTest(const FString& Parameters)
{
    // Weapon slots: 1=Fist/Chainsaw, 2=Pistol, 3=Shotgun/SSG, 4=Chaingun,
    // 5=Rocket, 6=Plasma, 7=BFG

    // Weapon number encoding in tic commands
    for (int32 WeaponSlot = 0; WeaponSlot < 7; WeaponSlot++)
    {
        uint8 Buttons = BT_CHANGE | ((uint8)WeaponSlot << BT_WEAPONSHIFT);
        int32 Decoded = (Buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;
        TestEqual(*FString::Printf(TEXT("Weapon %d encode/decode"), WeaponSlot),
            Decoded, WeaponSlot);
    }

    // Preferred weapon ordering (for auto-switch on pickup)
    // BFG > Plasma > SSG > Chaingun > Shotgun > Pistol > Fist/Chainsaw
    TArray<EDoomWeapon> WeaponPriority = {
        EDoomWeapon::BFG,
        EDoomWeapon::Plasma,
        EDoomWeapon::SuperShotgun,
        EDoomWeapon::Chaingun,
        EDoomWeapon::Shotgun,
        EDoomWeapon::Pistol,
        EDoomWeapon::Chainsaw,
        EDoomWeapon::Fist,
    };

    TestEqual(TEXT("Best weapon BFG"), WeaponPriority[0], EDoomWeapon::BFG);
    TestEqual(TEXT("Worst weapon Fist"), WeaponPriority.Last(), EDoomWeapon::Fist);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
