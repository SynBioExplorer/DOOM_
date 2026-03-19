#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.

#include "CoreMinimal.h"

// =============================================================================
// Fixed-point type aliases
// =============================================================================

/** DOOM fixed-point type: 16.16 format */
using fixed_t = int32;

/** DOOM binary angle type */
using angle_t = uint32;

// =============================================================================
// Fixed-point constants
// =============================================================================

constexpr int32 FRACBITS = 16;
constexpr fixed_t FRACUNIT = (1 << FRACBITS);

// =============================================================================
// Screen and timing constants
// =============================================================================

constexpr int32 SCREENWIDTH = 320;
constexpr int32 SCREENHEIGHT = 200;
constexpr int32 TICRATE = 35;
constexpr int32 MAXPLAYERS = 4;

// =============================================================================
// Game mode - identify IWAD version
// =============================================================================

UENUM(BlueprintType)
enum class EDoomGameMode : uint8
{
	Shareware = 0,      // DOOM 1 shareware, E1, M9
	Registered = 1,     // DOOM 1 registered, E3, M27
	Commercial = 2,     // DOOM 2 retail, E1 M34
	Retail = 3,         // DOOM 1 retail, E4, M36
	Indetermined = 4    // No IWAD found
};

// =============================================================================
// Mission packs
// =============================================================================

UENUM(BlueprintType)
enum class EDoomMission : uint8
{
	Doom = 0,           // DOOM 1
	Doom2 = 1,          // DOOM 2
	PackTNT = 2,        // TNT mission pack
	PackPlutonia = 3,   // Plutonia pack
	None = 4
};

// =============================================================================
// Language selection
// =============================================================================

UENUM(BlueprintType)
enum class EDoomLanguage : uint8
{
	English = 0,
	French = 1,
	German = 2,
	Unknown = 3
};

// =============================================================================
// Game state
// =============================================================================

UENUM(BlueprintType)
enum class EDoomGameState : uint8
{
	Level = 0,          // GS_LEVEL
	Intermission = 1,   // GS_INTERMISSION
	Finale = 2,         // GS_FINALE
	DemoScreen = 3      // GS_DEMOSCREEN
};

// =============================================================================
// Skill levels
// =============================================================================

UENUM(BlueprintType)
enum class EDoomSkill : uint8
{
	Baby = 0,           // sk_baby - I'm too young to die
	Easy = 1,           // sk_easy - Hey, not too rough
	Medium = 2,         // sk_medium - Hurt me plenty
	Hard = 3,           // sk_hard - Ultra-Violence
	Nightmare = 4       // sk_nightmare - Nightmare!
};

// =============================================================================
// Key cards
// =============================================================================

UENUM(BlueprintType)
enum class EDoomCard : uint8
{
	BlueCard = 0,       // it_bluecard
	YellowCard = 1,     // it_yellowcard
	RedCard = 2,        // it_redcard
	BlueSkull = 3,      // it_blueskull
	YellowSkull = 4,    // it_yellowskull
	RedSkull = 5,       // it_redskull
	NUMCARDS = 6
};

// =============================================================================
// Weapons
// =============================================================================

UENUM(BlueprintType)
enum class EDoomWeapon : uint8
{
	Fist = 0,           // wp_fist
	Pistol = 1,         // wp_pistol
	Shotgun = 2,        // wp_shotgun
	Chaingun = 3,       // wp_chaingun
	Missile = 4,        // wp_missile
	Plasma = 5,         // wp_plasma
	BFG = 6,            // wp_bfg
	Chainsaw = 7,       // wp_chainsaw
	SuperShotgun = 8,   // wp_supershotgun
	NUMWEAPONS = 9,
	NoChange = 10       // wp_nochange - No pending weapon change
};

// =============================================================================
// Ammunition types
// =============================================================================

UENUM(BlueprintType)
enum class EDoomAmmo : uint8
{
	Clip = 0,           // am_clip - Pistol / chaingun ammo
	Shell = 1,          // am_shell - Shotgun / double barreled shotgun
	Cell = 2,           // am_cell - Plasma rifle, BFG
	Missile = 3,        // am_misl - Missile launcher
	NUMAMMO = 4,
	NoAmmo = 5          // am_noammo - Unlimited for chainsaw / fist
};

// =============================================================================
// Power up types
// =============================================================================

UENUM(BlueprintType)
enum class EDoomPower : uint8
{
	Invulnerability = 0,  // pw_invulnerability
	Strength = 1,         // pw_strength
	Invisibility = 2,     // pw_invisibility
	IronFeet = 3,         // pw_ironfeet
	AllMap = 4,           // pw_allmap
	Infrared = 5,         // pw_infrared
	NUMPOWERS = 6
};

// =============================================================================
// Power up durations (in tics, assuming TICRATE=35)
// =============================================================================

constexpr int32 INVULNTICS = 30 * TICRATE;   // 1050
constexpr int32 INVISTICS  = 60 * TICRATE;   // 2100
constexpr int32 INFRATICS  = 120 * TICRATE;  // 4200
constexpr int32 IRONTICS   = 60 * TICRATE;   // 2100

// =============================================================================
// Event types
// =============================================================================

UENUM(BlueprintType)
enum class EDoomEventType : uint8
{
	KeyDown = 0,        // ev_keydown
	KeyUp = 1,          // ev_keyup
	Mouse = 2,          // ev_mouse
	Joystick = 3        // ev_joystick
};

// =============================================================================
// Game action
// =============================================================================

UENUM(BlueprintType)
enum class EDoomGameAction : uint8
{
	Nothing = 0,        // ga_nothing
	LoadLevel = 1,      // ga_loadlevel
	NewGame = 2,        // ga_newgame
	LoadGame = 3,       // ga_loadgame
	SaveGame = 4,       // ga_savegame
	PlayDemo = 5,       // ga_playdemo
	Completed = 6,      // ga_completed
	Victory = 7,        // ga_victory
	WorldDone = 8,      // ga_worlddone
	Screenshot = 9      // ga_screenshot
};

// =============================================================================
// Player states
// =============================================================================

UENUM(BlueprintType)
enum class EDoomPlayerState : uint8
{
	Live = 0,           // PST_LIVE - Playing or camping
	Dead = 1,           // PST_DEAD - Dead on the ground, view follows killer
	Reborn = 2          // PST_REBORN - Ready to restart/respawn
};

// =============================================================================
// Cheat flags (player internal flags)
// =============================================================================

UENUM(BlueprintType, Meta = (Bitflags))
enum class EDoomCheatFlags : uint8
{
	None       = 0,
	NoClip     = 1,     // CF_NOCLIP - No clipping, walk through barriers
	GodMode    = 2,     // CF_GODMODE - No damage, no health loss
	NoMomentum = 4      // CF_NOMOMENTUM - Debug aid
};
ENUM_CLASS_FLAGS(EDoomCheatFlags);

// =============================================================================
// Button/action code constants
// =============================================================================

constexpr int32 BT_ATTACK      = 1;
constexpr int32 BT_USE         = 2;
constexpr int32 BT_SPECIAL     = 128;
constexpr int32 BT_SPECIALMASK = 3;
constexpr int32 BT_CHANGE      = 4;
constexpr int32 BT_WEAPONMASK  = (8 + 16 + 32);
constexpr int32 BT_WEAPONSHIFT = 3;
constexpr int32 BTS_PAUSE      = 1;
constexpr int32 BTS_SAVEGAME   = 2;
constexpr int32 BTS_SAVEMASK   = (4 + 8 + 16);
constexpr int32 BTS_SAVESHIFT  = 2;

// =============================================================================
// Map object flags (mobjflag_t) - bit flags
// =============================================================================

constexpr int32 MF_SPECIAL      = 1;           // Call P_SpecialThing when touched
constexpr int32 MF_SOLID        = 2;           // Blocks
constexpr int32 MF_SHOOTABLE    = 4;           // Can be hit
constexpr int32 MF_NOSECTOR     = 8;           // Don't use the sector links
constexpr int32 MF_NOBLOCKMAP   = 16;          // Don't use the blocklinks
constexpr int32 MF_AMBUSH       = 32;          // Not activated by sound, deaf monster
constexpr int32 MF_JUSTHIT      = 64;          // Will try to attack right back
constexpr int32 MF_JUSTATTACKED = 128;         // Will take at least one step before attacking
constexpr int32 MF_SPAWNCEILING = 256;         // Hang from ceiling on spawn
constexpr int32 MF_NOGRAVITY    = 512;         // Don't apply gravity
constexpr int32 MF_DROPOFF      = 0x400;       // Allows jumps from high places
constexpr int32 MF_PICKUP       = 0x800;       // For players, will pick up items
constexpr int32 MF_NOCLIP       = 0x1000;      // Player cheat
constexpr int32 MF_SLIDE        = 0x2000;      // Keep info about sliding along walls
constexpr int32 MF_FLOAT        = 0x4000;      // Allow moves to any height, no gravity
constexpr int32 MF_TELEPORT     = 0x8000;      // Don't cross lines or look at heights on teleport
constexpr int32 MF_MISSILE      = 0x10000;     // Don't hit same species, explode on block
constexpr int32 MF_DROPPED      = 0x20000;     // Dropped by a demon, not level spawned
constexpr int32 MF_SHADOW       = 0x40000;     // Use fuzzy draw (shadow demons/spectres)
constexpr int32 MF_NOBLOOD      = 0x80000;     // Don't bleed when shot (use puff)
constexpr int32 MF_CORPSE       = 0x100000;    // Don't stop moving halfway off a step
constexpr int32 MF_INFLOAT      = 0x200000;    // Floating to a height for a move
constexpr int32 MF_COUNTKILL    = 0x400000;    // Count towards intermission kill total
constexpr int32 MF_COUNTITEM    = 0x800000;    // Count towards intermission item total
constexpr int32 MF_SKULLFLY     = 0x1000000;   // Special handling: skull in flight
constexpr int32 MF_NOTDMATCH    = 0x2000000;   // Don't spawn in deathmatch mode
constexpr int32 MF_TRANSLATION  = 0xc000000;   // Player color translation mask
constexpr int32 MF_TRANSSHIFT   = 26;          // Shift for translation bits

// =============================================================================
// Skill thing flags
// =============================================================================

constexpr int32 MTF_EASY   = 1;
constexpr int32 MTF_NORMAL = 2;
constexpr int32 MTF_HARD   = 4;
constexpr int32 MTF_AMBUSH = 8;

// =============================================================================
// DOOM keyboard definitions
// =============================================================================

constexpr int32 KEY_RIGHTARROW = 0xae;
constexpr int32 KEY_LEFTARROW  = 0xac;
constexpr int32 KEY_UPARROW    = 0xad;
constexpr int32 KEY_DOWNARROW  = 0xaf;
constexpr int32 KEY_ESCAPE     = 27;
constexpr int32 KEY_ENTER      = 13;
constexpr int32 KEY_TAB        = 9;
constexpr int32 KEY_F1         = (0x80 + 0x3b);
constexpr int32 KEY_F2         = (0x80 + 0x3c);
constexpr int32 KEY_F3         = (0x80 + 0x3d);
constexpr int32 KEY_F4         = (0x80 + 0x3e);
constexpr int32 KEY_F5         = (0x80 + 0x3f);
constexpr int32 KEY_F6         = (0x80 + 0x40);
constexpr int32 KEY_F7         = (0x80 + 0x41);
constexpr int32 KEY_F8         = (0x80 + 0x42);
constexpr int32 KEY_F9         = (0x80 + 0x43);
constexpr int32 KEY_F10        = (0x80 + 0x44);
constexpr int32 KEY_F11        = (0x80 + 0x57);
constexpr int32 KEY_F12        = (0x80 + 0x58);
constexpr int32 KEY_BACKSPACE  = 127;
constexpr int32 KEY_PAUSE      = 0xff;
constexpr int32 KEY_EQUALS     = 0x3d;
constexpr int32 KEY_MINUS      = 0x2d;
constexpr int32 KEY_RSHIFT     = (0x80 + 0x36);
constexpr int32 KEY_RCTRL      = (0x80 + 0x1d);
constexpr int32 KEY_RALT       = (0x80 + 0x38);
constexpr int32 KEY_LALT       = KEY_RALT;

// =============================================================================
// Max events
// =============================================================================

constexpr int32 MAXEVENTS = 64;
