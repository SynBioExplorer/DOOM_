// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of mobjinfo[] table from info.c
// Monster/thing definitions used by the entity system.

#include "DoomEntityInfo.h"

// Static member initialization
TArray<FDoomMobjInfo> UDoomEntityInfo::MobjInfoTable;
bool UDoomEntityInfo::bInitialized = false;

const FDoomMobjInfo& UDoomEntityInfo::GetMobjInfo(EDoomMobjType Type)
{
	if (!bInitialized)
	{
		InitMobjInfoTable();
	}

	const int32 Index = static_cast<int32>(Type);
	if (Index >= 0 && Index < MobjInfoTable.Num())
	{
		return MobjInfoTable[Index];
	}

	// Return player info as fallback
	static FDoomMobjInfo DefaultInfo;
	return DefaultInfo;
}

int32 UDoomEntityInfo::GetNumMobjTypes()
{
	if (!bInitialized)
	{
		InitMobjInfoTable();
	}
	return MobjInfoTable.Num();
}

// =============================================================================
// InitMobjInfoTable - Populates the mobjinfo table
// Direct port of mobjinfo[] from info.c
// Format: DoomEdNum, SpawnState, SpawnHealth, SeeState, SeeSound,
//         ReactionTime, AttackSound, PainState, PainChance, PainSound,
//         MeleeState, MissileState, DeathState, XDeathState, DeathSound,
//         Speed, Radius, Height, Mass, Damage, ActiveSound, Flags, RaiseState
// =============================================================================

void UDoomEntityInfo::InitMobjInfoTable()
{
	if (bInitialized)
		return;

	bInitialized = true;

	const int32 NumTypes = static_cast<int32>(EDoomMobjType::NUMMOBJTYPES);
	MobjInfoTable.SetNum(NumTypes);

	// Zero-initialize all entries
	for (int32 i = 0; i < NumTypes; i++)
	{
		MobjInfoTable[i] = FDoomMobjInfo();
	}

	// Helper lambda to set up monster info entries
	auto SetInfo = [](EDoomMobjType Type, int32 DoomEdNum, int32 SpawnHealth,
		int32 SeeSound, int32 ReactionTime, int32 AttackSound,
		int32 PainChance, int32 PainSound, int32 DeathSound,
		int32 Speed, float Radius, float Height, int32 Mass,
		int32 Damage, int32 ActiveSound, int32 Flags,
		bool bHasMelee, bool bHasMissile, bool bHasRaise)
	{
		const int32 Idx = static_cast<int32>(Type);
		MobjInfoTable[Idx].DoomEdNum = DoomEdNum;
		MobjInfoTable[Idx].SpawnHealth = SpawnHealth;
		MobjInfoTable[Idx].SeeSound = SeeSound;
		MobjInfoTable[Idx].ReactionTime = ReactionTime;
		MobjInfoTable[Idx].AttackSound = AttackSound;
		MobjInfoTable[Idx].PainChance = PainChance;
		MobjInfoTable[Idx].PainSound = PainSound;
		MobjInfoTable[Idx].DeathSound = DeathSound;
		MobjInfoTable[Idx].Speed = Speed;
		MobjInfoTable[Idx].Radius = Radius;
		MobjInfoTable[Idx].Height = Height;
		MobjInfoTable[Idx].Mass = Mass;
		MobjInfoTable[Idx].Damage = Damage;
		MobjInfoTable[Idx].ActiveSound = ActiveSound;
		MobjInfoTable[Idx].Flags = Flags;
		MobjInfoTable[Idx].MeleeState = bHasMelee ? 1 : 0;
		MobjInfoTable[Idx].MissileState = bHasMissile ? 1 : 0;
		MobjInfoTable[Idx].RaiseState = bHasRaise ? 1 : 0;
	};

	// Flags constants (matching MF_ from DoomTypes.h)
	constexpr int32 MF_SPECIAL      = 1;
	constexpr int32 MF_SOLID        = 2;
	constexpr int32 MF_SHOOTABLE    = 4;
	constexpr int32 MF_NOSECTOR     = 8;
	constexpr int32 MF_NOBLOCKMAP   = 16;
	constexpr int32 MF_AMBUSH       = 32;
	constexpr int32 MF_FLOAT        = 0x4000;
	constexpr int32 MF_MISSILE      = 0x10000;
	constexpr int32 MF_DROPPED      = 0x20000;
	constexpr int32 MF_SHADOW       = 0x40000;
	constexpr int32 MF_NOBLOOD      = 0x80000;
	constexpr int32 MF_COUNTKILL    = 0x400000;
	constexpr int32 MF_COUNTITEM    = 0x800000;
	constexpr int32 MF_SKULLFLY     = 0x1000000;
	constexpr int32 MF_NOGRAVITY    = 512;
	constexpr int32 MF_DROPOFF      = 0x400;
	constexpr int32 MF_PICKUP       = 0x800;
	constexpr int32 MF_SPAWNCEILING = 256;

	// =========================================================================
	// Monsters - exact values from info.c mobjinfo[]
	// =========================================================================

	// MT_PLAYER (DoomEdNum: -1)
	SetInfo(EDoomMobjType::MT_PLAYER, -1, 100,
		0, 0, 0, 255, 0, 0,
		0, 16.0f, 56.0f, 100, 0, 0,
		MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_PICKUP,
		false, false, false);

	// MT_POSSESSED - Zombieman (DoomEdNum: 3004)
	SetInfo(EDoomMobjType::MT_POSSESSED, 3004, 20,
		1, 8, 0, 200, 2, 3,
		8, 20.0f, 56.0f, 100, 0, 4,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, true);

	// MT_SHOTGUY - Shotgun Guy (DoomEdNum: 9)
	SetInfo(EDoomMobjType::MT_SHOTGUY, 9, 30,
		5, 8, 0, 170, 6, 7,
		8, 20.0f, 56.0f, 100, 0, 8,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, true);

	// MT_VILE - Arch-Vile (DoomEdNum: 64)
	SetInfo(EDoomMobjType::MT_VILE, 64, 700,
		9, 8, 0, 10, 10, 11,
		15, 20.0f, 56.0f, 500, 0, 12,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, true);

	// MT_UNDEAD - Revenant (DoomEdNum: 66)
	SetInfo(EDoomMobjType::MT_UNDEAD, 66, 300,
		13, 8, 0, 100, 14, 15,
		10, 20.0f, 56.0f, 500, 0, 16,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		true, true, true);

	// MT_FATSO - Mancubus (DoomEdNum: 67)
	SetInfo(EDoomMobjType::MT_FATSO, 67, 600,
		17, 8, 0, 80, 18, 19,
		8, 48.0f, 64.0f, 1000, 0, 20,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, true);

	// MT_CHAINGUY - Chaingunner (DoomEdNum: 65)
	SetInfo(EDoomMobjType::MT_CHAINGUY, 65, 70,
		21, 8, 0, 170, 22, 23,
		8, 20.0f, 56.0f, 100, 0, 24,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, true);

	// MT_TROOP - Imp (DoomEdNum: 3001)
	SetInfo(EDoomMobjType::MT_TROOP, 3001, 60,
		25, 8, 0, 200, 26, 27,
		8, 20.0f, 56.0f, 100, 0, 28,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		true, true, true);

	// MT_SERGEANT - Demon/Pinky (DoomEdNum: 3002)
	SetInfo(EDoomMobjType::MT_SERGEANT, 3002, 150,
		29, 8, 30, 180, 31, 32,
		10, 30.0f, 56.0f, 400, 0, 33,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		true, false, true);

	// MT_SHADOWS - Spectre (DoomEdNum: 58)
	SetInfo(EDoomMobjType::MT_SHADOWS, 58, 150,
		29, 8, 30, 180, 31, 32,
		10, 30.0f, 56.0f, 400, 0, 33,
		MF_SOLID | MF_SHOOTABLE | MF_SHADOW | MF_COUNTKILL,
		true, false, true);

	// MT_HEAD - Cacodemon (DoomEdNum: 3005)
	SetInfo(EDoomMobjType::MT_HEAD, 3005, 400,
		34, 8, 0, 128, 35, 36,
		8, 31.0f, 56.0f, 400, 0, 37,
		MF_SOLID | MF_SHOOTABLE | MF_FLOAT | MF_NOGRAVITY | MF_COUNTKILL,
		false, true, true);

	// MT_BRUISER - Baron of Hell (DoomEdNum: 3003)
	SetInfo(EDoomMobjType::MT_BRUISER, 3003, 1000,
		38, 8, 0, 50, 39, 40,
		8, 24.0f, 64.0f, 1000, 0, 41,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		true, true, true);

	// MT_KNIGHT - Hell Knight (DoomEdNum: 69)
	SetInfo(EDoomMobjType::MT_KNIGHT, 69, 500,
		38, 8, 0, 50, 39, 40,
		8, 24.0f, 64.0f, 1000, 0, 41,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		true, true, true);

	// MT_SKULL - Lost Soul (DoomEdNum: 3006)
	SetInfo(EDoomMobjType::MT_SKULL, 3006, 100,
		0, 8, 42, 256, 43, 44,
		8, 16.0f, 56.0f, 50, 3, 45,
		MF_SOLID | MF_SHOOTABLE | MF_FLOAT | MF_NOGRAVITY,
		false, true, false);

	// MT_SPIDER - Spider Mastermind (DoomEdNum: 7)
	SetInfo(EDoomMobjType::MT_SPIDER, 7, 3000,
		46, 8, 0, 40, 47, 48,
		12, 128.0f, 100.0f, 1000, 0, 49,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, false);

	// MT_BABY - Arachnotron (DoomEdNum: 68)
	SetInfo(EDoomMobjType::MT_BABY, 68, 500,
		50, 8, 0, 128, 51, 52,
		12, 64.0f, 64.0f, 600, 0, 53,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, false);

	// MT_CYBORG - Cyberdemon (DoomEdNum: 16)
	SetInfo(EDoomMobjType::MT_CYBORG, 16, 4000,
		54, 8, 0, 20, 55, 56,
		16, 40.0f, 110.0f, 1000, 0, 57,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, true, false);

	// MT_PAIN - Pain Elemental (DoomEdNum: 71)
	SetInfo(EDoomMobjType::MT_PAIN, 71, 400,
		58, 8, 0, 128, 59, 60,
		8, 31.0f, 56.0f, 400, 0, 61,
		MF_SOLID | MF_SHOOTABLE | MF_FLOAT | MF_NOGRAVITY | MF_COUNTKILL,
		false, true, false);

	// MT_KEEN - Commander Keen (DoomEdNum: 72)
	SetInfo(EDoomMobjType::MT_KEEN, 72, 100,
		0, 8, 0, 256, 62, 63,
		0, 16.0f, 72.0f, 10000000, 0, 0,
		MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL,
		false, false, false);

	// MT_BOSSBRAIN - Icon of Sin brain (DoomEdNum: 88)
	SetInfo(EDoomMobjType::MT_BOSSBRAIN, 88, 250,
		0, 8, 0, 255, 64, 65,
		0, 16.0f, 16.0f, 10000000, 0, 0,
		MF_SOLID | MF_SHOOTABLE,
		false, false, false);

	// MT_BOSSSPIT - Icon of Sin eye (DoomEdNum: 89)
	SetInfo(EDoomMobjType::MT_BOSSSPIT, 89, 1000,
		0, 0, 0, 0, 0, 0,
		0, 20.0f, 32.0f, 100, 0, 0,
		MF_NOBLOCKMAP | MF_NOSECTOR,
		false, false, false);

	// MT_BARREL - Explosive barrel (DoomEdNum: 2035)
	SetInfo(EDoomMobjType::MT_BARREL, 2035, 20,
		0, 8, 0, 0, 0, 66,
		0, 10.0f, 42.0f, 100, 0, 0,
		MF_SOLID | MF_SHOOTABLE | MF_NOBLOOD,
		false, false, false);

	// =========================================================================
	// Projectiles
	// =========================================================================

	// MT_TROOPSHOT - Imp fireball
	SetInfo(EDoomMobjType::MT_TROOPSHOT, -1, 1000,
		0, 8, 0, 0, 0, 0,
		10, 6.0f, 8.0f, 100, 3, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// MT_HEADSHOT - Cacodemon ball
	SetInfo(EDoomMobjType::MT_HEADSHOT, -1, 1000,
		0, 8, 0, 0, 0, 0,
		10, 6.0f, 8.0f, 100, 5, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// MT_BRUISERSHOT - Baron/Knight fireball
	SetInfo(EDoomMobjType::MT_BRUISERSHOT, -1, 1000,
		0, 8, 0, 0, 0, 0,
		15, 6.0f, 8.0f, 100, 8, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// MT_ROCKET - Rocket projectile
	SetInfo(EDoomMobjType::MT_ROCKET, -1, 1000,
		0, 8, 0, 0, 0, 67,
		20, 11.0f, 8.0f, 100, 20, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// MT_ARACHPLAZ - Arachnotron plasma
	SetInfo(EDoomMobjType::MT_ARACHPLAZ, -1, 1000,
		0, 8, 0, 0, 0, 0,
		25, 13.0f, 8.0f, 100, 5, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// MT_FATSHOT - Mancubus fireball
	SetInfo(EDoomMobjType::MT_FATSHOT, -1, 1000,
		0, 8, 0, 0, 0, 0,
		20, 6.0f, 8.0f, 100, 8, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// MT_TRACER - Revenant homing missile
	SetInfo(EDoomMobjType::MT_TRACER, -1, 1000,
		0, 8, 0, 0, 0, 0,
		10, 11.0f, 8.0f, 100, 10, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// MT_SPAWNSHOT - Icon of Sin spawn cube
	SetInfo(EDoomMobjType::MT_SPAWNSHOT, -1, 1000,
		0, 8, 0, 0, 0, 0,
		10, 6.0f, 8.0f, 100, 3, 0,
		MF_NOBLOCKMAP | MF_MISSILE | MF_DROPOFF | MF_NOGRAVITY,
		false, false, false);

	// =========================================================================
	// Pickup items (simplified - just DoomEdNum, health=1000, and flags)
	// =========================================================================

	auto SetPickup = [](EDoomMobjType Type, int32 DoomEdNum, float Radius, float Height)
	{
		const int32 Idx = static_cast<int32>(Type);
		MobjInfoTable[Idx].DoomEdNum = DoomEdNum;
		MobjInfoTable[Idx].SpawnHealth = 1000;
		MobjInfoTable[Idx].Radius = Radius;
		MobjInfoTable[Idx].Height = Height;
		MobjInfoTable[Idx].Flags = 1 /*MF_SPECIAL*/ | 0x800000 /*MF_COUNTITEM*/;
	};

	SetPickup(EDoomMobjType::MT_MISC0, 2018, 20.0f, 16.0f);   // Green Armor
	SetPickup(EDoomMobjType::MT_MISC1, 2019, 20.0f, 16.0f);   // Blue Armor
	SetPickup(EDoomMobjType::MT_MISC2, 2014, 20.0f, 16.0f);   // Health Bonus
	SetPickup(EDoomMobjType::MT_MISC3, 2015, 20.0f, 16.0f);   // Armor Bonus
	SetPickup(EDoomMobjType::MT_MISC4, 5, 20.0f, 16.0f);      // Blue Keycard
	SetPickup(EDoomMobjType::MT_MISC5, 13, 20.0f, 16.0f);     // Red Keycard
	SetPickup(EDoomMobjType::MT_MISC6, 6, 20.0f, 16.0f);      // Yellow Keycard
	SetPickup(EDoomMobjType::MT_MISC10, 2011, 20.0f, 16.0f);  // Stimpack
	SetPickup(EDoomMobjType::MT_MISC11, 2012, 20.0f, 16.0f);  // Medikit
	SetPickup(EDoomMobjType::MT_MISC12, 2013, 20.0f, 16.0f);  // Soulsphere
	SetPickup(EDoomMobjType::MT_INV, 2022, 20.0f, 16.0f);     // Invulnerability
	SetPickup(EDoomMobjType::MT_MISC13, 2023, 20.0f, 16.0f);  // Berserk
	SetPickup(EDoomMobjType::MT_INS, 2024, 20.0f, 16.0f);     // Invisibility
	SetPickup(EDoomMobjType::MT_MEGA, 83, 20.0f, 16.0f);      // Megasphere
	SetPickup(EDoomMobjType::MT_CLIP, 2007, 20.0f, 16.0f);    // Ammo Clip
	SetPickup(EDoomMobjType::MT_MISC25, 2006, 20.0f, 16.0f);  // BFG 9000
	SetPickup(EDoomMobjType::MT_CHAINGUN, 2002, 20.0f, 16.0f);
	SetPickup(EDoomMobjType::MT_SHOTGUN, 2001, 20.0f, 16.0f);
	SetPickup(EDoomMobjType::MT_SUPERSHOTGUN, 82, 20.0f, 16.0f);
}
