#pragma once

#include "CoreMinimal.h"

// =============================================================================
// EDoomMobjType - Direct port of mobjtype_t from info.h
// All ~140 types from the original DOOM source
// =============================================================================
UENUM(BlueprintType)
enum class EDoomMobjType : uint8
{
	MT_PLAYER = 0,
	MT_POSSESSED,
	MT_SHOTGUY,
	MT_VILE,
	MT_FIRE,
	MT_UNDEAD,
	MT_TRACER,
	MT_SMOKE,
	MT_FATSO,
	MT_FATSHOT,
	MT_CHAINGUY,
	MT_TROOP,       // Imp
	MT_SERGEANT,    // Demon (Pinky)
	MT_SHADOWS,     // Spectre
	MT_HEAD,        // Cacodemon
	MT_BRUISER,     // Baron of Hell
	MT_BRUISERSHOT,
	MT_KNIGHT,      // Hell Knight
	MT_SKULL,       // Lost Soul
	MT_SPIDER,      // Spider Mastermind
	MT_BABY,        // Arachnotron
	MT_CYBORG,      // Cyberdemon
	MT_PAIN,        // Pain Elemental
	MT_WOLFSS,
	MT_KEEN,
	MT_BOSSBRAIN,
	MT_BOSSSPIT,
	MT_BOSSTARGET,
	MT_SPAWNSHOT,
	MT_SPAWNFIRE,
	MT_BARREL,
	MT_TROOPSHOT,
	MT_HEADSHOT,
	MT_ROCKET,
	MT_PLASMA,
	MT_BFG,
	MT_ARACHPLAZ,
	MT_PUFF,
	MT_BLOOD,
	MT_TFOG,
	MT_IFOG,
	MT_TELEPORTMAN,
	MT_EXTRABFG,
	MT_MISC0,       // Green Armor
	MT_MISC1,       // Blue Armor
	MT_MISC2,       // Health Bonus
	MT_MISC3,       // Armor Bonus
	MT_MISC4,       // Blue Keycard
	MT_MISC5,       // Red Keycard
	MT_MISC6,       // Yellow Keycard
	MT_MISC7,       // Yellow Skull Key
	MT_MISC8,       // Red Skull Key
	MT_MISC9,       // Blue Skull Key
	MT_MISC10,      // Stimpack
	MT_MISC11,      // Medikit
	MT_MISC12,      // Soulsphere
	MT_INV,         // Invulnerability
	MT_MISC13,      // Berserk
	MT_INS,         // Invisibility
	MT_MISC14,      // Radiation Suit
	MT_MISC15,      // Computer Map
	MT_MISC16,      // Light Amp Goggles
	MT_MEGA,        // Megasphere
	MT_CLIP,        // Ammo Clip
	MT_MISC17,      // Box of Ammo
	MT_MISC18,      // Rocket
	MT_MISC19,      // Box of Rockets
	MT_MISC20,      // Cell Charge
	MT_MISC21,      // Cell Charge Pack
	MT_MISC22,      // Shells
	MT_MISC23,      // Box of Shells
	MT_MISC24,      // Backpack
	MT_MISC25,      // BFG 9000
	MT_CHAINGUN,
	MT_MISC26,      // Chainsaw
	MT_MISC27,      // Rocket Launcher
	MT_MISC28,      // Plasma Rifle
	MT_SHOTGUN,
	MT_SUPERSHOTGUN,
	MT_MISC29,      // Tall techno floor lamp
	MT_MISC30,      // Short techno floor lamp
	MT_MISC31,      // Floor lamp
	MT_MISC32,      // Tall green pillar
	MT_MISC33,      // Short green pillar
	MT_MISC34,      // Tall red pillar
	MT_MISC35,      // Short red pillar
	MT_MISC36,      // Short red pillar (skull)
	MT_MISC37,      // Short green pillar (heart)
	MT_MISC38,      // Evil eye
	MT_MISC39,      // Floating skull
	MT_MISC40,      // Burnt tree
	MT_MISC41,      // Candelabra
	MT_MISC42,      // Tall brown pillar
	MT_MISC43,      // Tall brown pillar2
	MT_MISC44,      // Short brown pillar
	MT_MISC45,      // Tall blue torch
	MT_MISC46,      // Tall green torch
	MT_MISC47,      // Tall red torch
	MT_MISC48,      // Short blue torch
	MT_MISC49,      // Short green torch
	MT_MISC50,      // Short red torch
	MT_MISC51,      // Hanging body 1
	MT_MISC52,      // Hanging body 2
	MT_MISC53,      // Hanging body 3
	MT_MISC54,      // Hanging body 4
	MT_MISC55,      // Hanging body 5
	MT_MISC56,      // Hanging body 6
	MT_MISC57,      // Dead cacodemon
	MT_MISC58,      // Dead player
	MT_MISC59,      // Dead possessed
	MT_MISC60,      // Dead imp
	MT_MISC61,      // Dead demon
	MT_MISC62,      // Dead lost soul (invisible)
	MT_MISC63,      // Dead possessed 2
	MT_MISC64,      // Dead imp 2
	MT_MISC65,      // Dead possessed 3
	MT_MISC66,      // Pool of blood 1
	MT_MISC67,      // Pool of blood 2
	MT_MISC68,      // Pool of blood 3
	MT_MISC69,      // Pool of blood 4
	MT_MISC70,      // Dead marine
	MT_MISC71,
	MT_MISC72,
	MT_MISC73,
	MT_MISC74,
	MT_MISC75,
	MT_MISC76,
	MT_MISC77,
	MT_MISC78,
	MT_MISC79,
	MT_MISC80,
	MT_MISC81,
	MT_MISC82,
	MT_MISC83,
	MT_MISC84,
	MT_MISC85,
	MT_MISC86,

	NUMMOBJTYPES
};

// =============================================================================
// EDoomMobjFlag - Direct port of mobjflag_t from p_mobj.h
// =============================================================================
UENUM(BlueprintType, Meta = (Bitflags))
enum class EDoomMobjFlag : int32
{
	MF_SPECIAL       = 1,
	MF_SOLID         = 2,
	MF_SHOOTABLE     = 4,
	MF_NOSECTOR      = 8,
	MF_NOBLOCKMAP    = 16,
	MF_AMBUSH        = 32,
	MF_JUSTHIT       = 64,
	MF_JUSTATTACKED  = 128,
	MF_SPAWNCEILING  = 256,
	MF_NOGRAVITY     = 512,
	MF_DROPOFF       = 0x400,
	MF_PICKUP        = 0x800,
	MF_NOCLIP        = 0x1000,
	MF_SLIDE         = 0x2000,
	MF_FLOAT         = 0x4000,
	MF_TELEPORT      = 0x8000,
	MF_MISSILE       = 0x10000,
	MF_DROPPED       = 0x20000,
	MF_SHADOW        = 0x40000,
	MF_NOBLOOD       = 0x80000,
	MF_CORPSE        = 0x100000,
	MF_INFLOAT       = 0x200000,
	MF_COUNTKILL     = 0x400000,
	MF_COUNTITEM     = 0x800000,
	MF_SKULLFLY      = 0x1000000,
	MF_NOTDMATCH     = 0x2000000,
	MF_TRANSLATION   = 0xc000000
};

// =============================================================================
// FDoomMobjInfo - Direct port of mobjinfo_t
// =============================================================================
USTRUCT(BlueprintType)
struct FDoomMobjInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DoomEdNum = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SpawnState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SpawnHealth = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SeeState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SeeSound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ReactionTime = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AttackSound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PainState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PainChance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PainSound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MeleeState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MissileState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DeathState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 XDeathState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DeathSound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Speed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Mass = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Damage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ActiveSound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Flags = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RaiseState = 0;
};

// =============================================================================
// UDoomEntityInfo - Static lookup for entity info tables
// =============================================================================
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomEntityInfo : public UObject
{
	GENERATED_BODY()

public:
	// Get the info for a given mobj type
	UFUNCTION(BlueprintCallable, Category = "Doom|EntityInfo")
	static const FDoomMobjInfo& GetMobjInfo(EDoomMobjType Type);

	// Get the total number of mobj types
	UFUNCTION(BlueprintCallable, Category = "Doom|EntityInfo")
	static int32 GetNumMobjTypes();

	// Convert DOOM fixed-point radius/height to Unreal units
	// DOOM: 1 unit ~ 1 map unit. Unreal: 1 unit = 1 cm.
	// Scale factor: roughly 1 DOOM unit = 1.2 UE units for gameplay feel
	static constexpr float DOOM_TO_UNREAL_SCALE = 1.2f;
	static constexpr float FIXED_TO_FLOAT = 1.0f / 65536.0f;

	// Initialize the mobjinfo table (called once at startup)
	static void InitMobjInfoTable();

private:
	static TArray<FDoomMobjInfo> MobjInfoTable;
	static bool bInitialized;
};
