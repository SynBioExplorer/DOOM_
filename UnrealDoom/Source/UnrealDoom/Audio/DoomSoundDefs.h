#pragma once

#include "CoreMinimal.h"

// =============================================================================
// EDoomSfx - All sound effect identifiers from DOOM
// Direct port of sfxenum_t from sounds.h
// =============================================================================

UENUM(BlueprintType)
enum class EDoomSfx : uint8
{
	None = 0,

	// Weapons
	Pistol,        // sfx_pistol
	Shotgn,        // sfx_shotgn - shotgun fire
	Sgcock,        // sfx_sgcock - shotgun cock
	Dshtgn,        // sfx_dshtgn - double-barrel shotgun fire
	Dbopn,         // sfx_dbopn  - double-barrel open
	Dbcls,         // sfx_dbcls  - double-barrel close
	Dbload,        // sfx_dbload - double-barrel load
	Plasma,        // sfx_plasma - plasma rifle
	Bfg,           // sfx_bfg    - BFG fire
	Sawup,         // sfx_sawup  - chainsaw start
	Sawidl,        // sfx_sawidl - chainsaw idle
	Sawful,        // sfx_sawful - chainsaw full throttle
	Sawhit,        // sfx_sawhit - chainsaw hit
	Rlaunc,        // sfx_rlaunc - rocket launch
	Rxplod,        // sfx_rxplod - rocket explosion
	Firsht,        // sfx_firsht - fireball shot
	Firxpl,        // sfx_firxpl - fireball explosion

	// Environment
	Pstart,        // sfx_pstart - platform start
	Pstop,         // sfx_pstop  - platform stop
	Doropn,        // sfx_doropn - door open
	Dorcls,        // sfx_dorcls - door close
	Stnmov,        // sfx_stnmov - stone moving
	Swtchn,        // sfx_swtchn - switch on
	Swtchx,        // sfx_swtchx - switch off

	// Player
	Plpain,        // sfx_plpain - player pain
	Dmpain,        // sfx_dmpain - demon pain
	Popain,        // sfx_popain - possessed pain
	Vipain,        // sfx_vipain - archvile pain
	Mnpain,        // sfx_mnpain - mancubus pain
	Pepain,        // sfx_pepain - pain elemental pain
	Slop,          // sfx_slop   - gib splat
	Itemup,        // sfx_itemup - item pickup
	Wpnup,         // sfx_wpnup  - weapon pickup
	Oof,           // sfx_oof    - player oof (wall bump)
	Telept,        // sfx_telept - teleport

	// Possessed / Zombieman
	Posit1,        // sfx_posit1 - possessed sight 1
	Posit2,        // sfx_posit2 - possessed sight 2
	Posit3,        // sfx_posit3 - possessed sight 3

	// Shotgun Guy
	Bgsit1,        // sfx_bgsit1 - shotgun guy sight 1
	Bgsit2,        // sfx_bgsit2 - shotgun guy sight 2

	// Imp / Sergeant
	Sgtsit,        // sfx_sgtsit - sergeant sight

	// Cacodemon
	Cacsit,        // sfx_cacsit - cacodemon sight

	// Baron of Hell
	Brssit,        // sfx_brssit - baron sight

	// Cyberdemon
	Cybsit,        // sfx_cybsit - cyberdemon sight

	// Spider Mastermind
	Spisit,        // sfx_spisit - spider mastermind sight

	// Arachnotron
	Bspsit,        // sfx_bspsit - arachnotron sight

	// Hell Knight
	Kntsit,        // sfx_kntsit - hell knight sight

	// Archvile
	Vilsit,        // sfx_vilsit - archvile sight

	// Mancubus
	Mansit,        // sfx_mansit - mancubus sight

	// Pain Elemental
	Pesit,         // sfx_pesit  - pain elemental sight

	// Attacks
	Sklatk,        // sfx_sklatk - skeleton attack
	Sgtatk,        // sfx_sgtatk - sergeant attack
	Skepch,        // sfx_skepch - skeleton punch
	Vilatk,        // sfx_vilatk - archvile attack
	Claw,          // sfx_claw   - claw attack
	Skeswg,        // sfx_skeswg - skeleton swing

	// Deaths
	Pldeth,        // sfx_pldeth - player death
	Pdiehi,        // sfx_pdiehi - player death (high pitch)
	Podth1,        // sfx_podth1 - possessed death 1
	Podth2,        // sfx_podth2 - possessed death 2
	Podth3,        // sfx_podth3 - possessed death 3
	Bgdth1,        // sfx_bgdth1 - shotgun guy death 1
	Bgdth2,        // sfx_bgdth2 - shotgun guy death 2
	Sgtdth,        // sfx_sgtdth - sergeant death
	Cacdth,        // sfx_cacdth - cacodemon death
	Skldth,        // sfx_skldth - skeleton death
	Brsdth,        // sfx_brsdth - baron death
	Cybdth,        // sfx_cybdth - cyberdemon death
	Spidth,        // sfx_spidth - spider mastermind death
	Bspdth,        // sfx_bspdth - arachnotron death
	Vildth,        // sfx_vildth - archvile death
	Kntdth,        // sfx_kntdth - hell knight death
	Pedth,         // sfx_pedth  - pain elemental death
	Skedth,        // sfx_skedth - skeleton death

	// Active sounds
	Posact,        // sfx_posact - possessed active
	Bgact,         // sfx_bgact  - shotgun guy active
	Dmact,         // sfx_dmact  - demon active
	Bspact,        // sfx_bspact - arachnotron active
	Bspwlk,        // sfx_bspwlk - arachnotron walk
	Vilact,        // sfx_vilact - archvile active

	// Misc
	Noway,         // sfx_noway  - can't use / blocked
	Barexp,        // sfx_barexp - barrel explosion
	Punch,         // sfx_punch  - fist punch
	Hoof,          // sfx_hoof   - cyberdemon hoof
	Metal,         // sfx_metal  - metal footstep
	Chgun,         // sfx_chgun  - chaingun (linked to pistol)
	Tink,          // sfx_tink   - notification tink
	Bdopn,         // sfx_bdopn  - blazing door open
	Bdcls,         // sfx_bdcls  - blazing door close
	Itmbk,         // sfx_itmbk  - item respawn
	Flame,         // sfx_flame  - flame
	Flamst,        // sfx_flamst - flame start
	Getpow,        // sfx_getpow - powerup pickup
	Bospit,        // sfx_bospit - boss spit
	Boscub,        // sfx_boscub - boss cube
	Bossit,        // sfx_bossit - boss sight
	Bospn,         // sfx_bospn  - boss pain
	Bosdth,        // sfx_bosdth - boss death
	Manatk,        // sfx_manatk - mancubus attack
	Mandth,        // sfx_mandth - mancubus death
	Sssit,         // sfx_sssit  - Wolfenstein SS sight
	Ssdth,         // sfx_ssdth  - Wolfenstein SS death
	Keenpn,        // sfx_keenpn - Commander Keen pain
	Keendt,        // sfx_keendt - Commander Keen death
	Skeact,        // sfx_skeact - skeleton active
	Skesit,        // sfx_skesit - skeleton sight
	Skeatk,        // sfx_skeatk - skeleton attack (ranged)
	Radio,         // sfx_radio  - radio static / message

	NumSfx         UMETA(Hidden)
};

// =============================================================================
// EDoomMusic - All music track identifiers from DOOM
// Direct port of musicenum_t from sounds.h
// =============================================================================

UENUM(BlueprintType)
enum class EDoomMusic : uint8
{
	None = 0,

	// DOOM Episode 1: Knee-Deep in the Dead
	E1M1,    // e1m1 - At Doom's Gate
	E1M2,    // e1m2 - The Imp's Song
	E1M3,    // e1m3 - Dark Halls
	E1M4,    // e1m4 - Kitchen Ace
	E1M5,    // e1m5 - Suspense
	E1M6,    // e1m6 - On the Hunt
	E1M7,    // e1m7 - Demons on the Prey
	E1M8,    // e1m8 - Sign of Evil
	E1M9,    // e1m9 - Hiding the Secrets

	// DOOM Episode 2: The Shores of Hell
	E2M1,    // e2m1
	E2M2,    // e2m2
	E2M3,    // e2m3
	E2M4,    // e2m4
	E2M5,    // e2m5
	E2M6,    // e2m6
	E2M7,    // e2m7
	E2M8,    // e2m8
	E2M9,    // e2m9

	// DOOM Episode 3: Inferno
	E3M1,    // e3m1
	E3M2,    // e3m2
	E3M3,    // e3m3
	E3M4,    // e3m4
	E3M5,    // e3m5
	E3M6,    // e3m6
	E3M7,    // e3m7
	E3M8,    // e3m8
	E3M9,    // e3m9

	// Intermission / special
	Inter,   // inter - intermission
	Intro,   // intro - intro screen
	Bunny,   // bunny - bunny scroll
	Victor,  // victor - victory
	Introa,  // introa - alternate intro

	// DOOM 2 music tracks (MAP01-MAP32)
	Runnin,  // runnin  - MAP01 Running From Evil
	Stalks,  // stalks  - MAP02 The Healer Stalks
	Countd,  // countd  - MAP03 Countdown to Death
	Betwee,  // betwee  - MAP04 Between Levels
	Doom,    // doom    - MAP05 DOOM
	The_Da,  // the_da  - MAP06 The Dave D. Taylor Blues
	Shawn,   // shawn   - MAP07 Shawn's Got the Shotgun
	Ddtblu,  // ddtblu  - MAP08 The Dave D. Taylor Blues (remix)
	In_Cit,  // in_cit  - MAP09 Into Sandy's City
	Dead,    // dead    - MAP10 The Demon's Dead
	Stlks2,  // stlks2  - MAP11
	Theda2,  // theda2  - MAP12
	Doom2,   // doom2   - MAP13
	Ddtbl2,  // ddtbl2  - MAP14
	Runni2,  // runni2  - MAP15
	Dead2,   // dead2   - MAP16
	Stlks3,  // stlks3  - MAP17
	Romero,  // romero  - MAP18
	Shawn2,  // shawn2  - MAP19
	Messag,  // messag  - MAP20 Message For the Archvile
	Count2,  // count2  - MAP21
	Ddtbl3,  // ddtbl3  - MAP22
	Ampie,   // ampie   - MAP23
	Theda3,  // theda3  - MAP24
	Adrian,  // adrian  - MAP25
	Messg2,  // messg2  - MAP26
	Romer2,  // romer2  - MAP27
	Tense,   // tense   - MAP28 The Demon's Dead
	Shawn3,  // shawn3  - MAP29
	Openin,  // openin  - MAP30 Opening to Hell
	Evil,    // evil    - MAP31
	Ultima,  // ultima  - MAP32
	Read_M,  // read_m  - text screen
	Dm2ttl,  // dm2ttl  - DOOM 2 title
	Dm2int,  // dm2int  - DOOM 2 intermission

	NumMusic UMETA(Hidden)
};

// =============================================================================
// FDoomSfxInfo - Sound effect definition data
// Mirrors sfxinfo_t from sounds.h
// =============================================================================

USTRUCT(BlueprintType)
struct FDoomSfxInfo
{
	GENERATED_BODY()

	// Sound name (maps to WAD lump: "DS" + Name)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	// If true, only one instance can play at a time (singularity)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSingularity = false;

	// Sound priority (lower = higher priority in original DOOM)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 64;

	// Linked sound for pitch/volume variations (INDEX_NONE if no link)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDoomSfx Link = EDoomSfx::None;

	// Pitch offset if linked
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PitchOffset = -1;

	// Volume offset if linked
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 VolumeOffset = -1;

	// UE5 sound asset reference
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<class USoundBase> SoundAsset;
};

// =============================================================================
// FDoomMusicInfo - Music track definition data
// Mirrors musicinfo_t from sounds.h
// =============================================================================

USTRUCT(BlueprintType)
struct FDoomMusicInfo
{
	GENERATED_BODY()

	// Music name (maps to WAD lump: "D_" + Name)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	// UE5 sound asset reference for music
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<class USoundBase> MusicAsset;
};

// =============================================================================
// Helper to get the WAD lump name for a sound/music enum value
// =============================================================================

namespace DoomSoundUtils
{
	/** Get the original DOOM WAD lump name for an SFX (e.g., "pistol", "shotgn") */
	inline FString GetSfxLumpName(EDoomSfx SfxId)
	{
		static const TMap<EDoomSfx, FString> SfxNames = {
			{ EDoomSfx::Pistol,  TEXT("pistol") },
			{ EDoomSfx::Shotgn,  TEXT("shotgn") },
			{ EDoomSfx::Sgcock,  TEXT("sgcock") },
			{ EDoomSfx::Dshtgn,  TEXT("dshtgn") },
			{ EDoomSfx::Dbopn,   TEXT("dbopn")  },
			{ EDoomSfx::Dbcls,   TEXT("dbcls")  },
			{ EDoomSfx::Dbload,  TEXT("dbload") },
			{ EDoomSfx::Plasma,  TEXT("plasma") },
			{ EDoomSfx::Bfg,     TEXT("bfg")    },
			{ EDoomSfx::Sawup,   TEXT("sawup")  },
			{ EDoomSfx::Sawidl,  TEXT("sawidl") },
			{ EDoomSfx::Sawful,  TEXT("sawful") },
			{ EDoomSfx::Sawhit,  TEXT("sawhit") },
			{ EDoomSfx::Rlaunc,  TEXT("rlaunc") },
			{ EDoomSfx::Rxplod,  TEXT("rxplod") },
			{ EDoomSfx::Firsht,  TEXT("firsht") },
			{ EDoomSfx::Firxpl,  TEXT("firxpl") },
			{ EDoomSfx::Pstart,  TEXT("pstart") },
			{ EDoomSfx::Pstop,   TEXT("pstop")  },
			{ EDoomSfx::Doropn,  TEXT("doropn") },
			{ EDoomSfx::Dorcls,  TEXT("dorcls") },
			{ EDoomSfx::Stnmov,  TEXT("stnmov") },
			{ EDoomSfx::Swtchn,  TEXT("swtchn") },
			{ EDoomSfx::Swtchx,  TEXT("swtchx") },
			{ EDoomSfx::Plpain,  TEXT("plpain") },
			{ EDoomSfx::Dmpain,  TEXT("dmpain") },
			{ EDoomSfx::Popain,  TEXT("popain") },
			{ EDoomSfx::Vipain,  TEXT("vipain") },
			{ EDoomSfx::Mnpain,  TEXT("mnpain") },
			{ EDoomSfx::Pepain,  TEXT("pepain") },
			{ EDoomSfx::Slop,    TEXT("slop")   },
			{ EDoomSfx::Itemup,  TEXT("itemup") },
			{ EDoomSfx::Wpnup,   TEXT("wpnup")  },
			{ EDoomSfx::Oof,     TEXT("oof")    },
			{ EDoomSfx::Telept,  TEXT("telept") },
			{ EDoomSfx::Posit1,  TEXT("posit1") },
			{ EDoomSfx::Posit2,  TEXT("posit2") },
			{ EDoomSfx::Posit3,  TEXT("posit3") },
			{ EDoomSfx::Bgsit1,  TEXT("bgsit1") },
			{ EDoomSfx::Bgsit2,  TEXT("bgsit2") },
			{ EDoomSfx::Sgtsit,  TEXT("sgtsit") },
			{ EDoomSfx::Cacsit,  TEXT("cacsit") },
			{ EDoomSfx::Brssit,  TEXT("brssit") },
			{ EDoomSfx::Cybsit,  TEXT("cybsit") },
			{ EDoomSfx::Spisit,  TEXT("spisit") },
			{ EDoomSfx::Bspsit,  TEXT("bspsit") },
			{ EDoomSfx::Kntsit,  TEXT("kntsit") },
			{ EDoomSfx::Vilsit,  TEXT("vilsit") },
			{ EDoomSfx::Mansit,  TEXT("mansit") },
			{ EDoomSfx::Pesit,   TEXT("pesit")  },
			{ EDoomSfx::Sklatk,  TEXT("sklatk") },
			{ EDoomSfx::Sgtatk,  TEXT("sgtatk") },
			{ EDoomSfx::Skepch,  TEXT("skepch") },
			{ EDoomSfx::Vilatk,  TEXT("vilatk") },
			{ EDoomSfx::Claw,    TEXT("claw")   },
			{ EDoomSfx::Skeswg,  TEXT("skeswg") },
			{ EDoomSfx::Pldeth,  TEXT("pldeth") },
			{ EDoomSfx::Pdiehi,  TEXT("pdiehi") },
			{ EDoomSfx::Podth1,  TEXT("podth1") },
			{ EDoomSfx::Podth2,  TEXT("podth2") },
			{ EDoomSfx::Podth3,  TEXT("podth3") },
			{ EDoomSfx::Bgdth1,  TEXT("bgdth1") },
			{ EDoomSfx::Bgdth2,  TEXT("bgdth2") },
			{ EDoomSfx::Sgtdth,  TEXT("sgtdth") },
			{ EDoomSfx::Cacdth,  TEXT("cacdth") },
			{ EDoomSfx::Skldth,  TEXT("skldth") },
			{ EDoomSfx::Brsdth,  TEXT("brsdth") },
			{ EDoomSfx::Cybdth,  TEXT("cybdth") },
			{ EDoomSfx::Spidth,  TEXT("spidth") },
			{ EDoomSfx::Bspdth,  TEXT("bspdth") },
			{ EDoomSfx::Vildth,  TEXT("vildth") },
			{ EDoomSfx::Kntdth,  TEXT("kntdth") },
			{ EDoomSfx::Pedth,   TEXT("pedth")  },
			{ EDoomSfx::Skedth,  TEXT("skedth") },
			{ EDoomSfx::Posact,  TEXT("posact") },
			{ EDoomSfx::Bgact,   TEXT("bgact")  },
			{ EDoomSfx::Dmact,   TEXT("dmact")  },
			{ EDoomSfx::Bspact,  TEXT("bspact") },
			{ EDoomSfx::Bspwlk,  TEXT("bspwlk") },
			{ EDoomSfx::Vilact,  TEXT("vilact") },
			{ EDoomSfx::Noway,   TEXT("noway")  },
			{ EDoomSfx::Barexp,  TEXT("barexp") },
			{ EDoomSfx::Punch,   TEXT("punch")  },
			{ EDoomSfx::Hoof,    TEXT("hoof")   },
			{ EDoomSfx::Metal,   TEXT("metal")  },
			{ EDoomSfx::Chgun,   TEXT("chgun")  },
			{ EDoomSfx::Tink,    TEXT("tink")   },
			{ EDoomSfx::Bdopn,   TEXT("bdopn")  },
			{ EDoomSfx::Bdcls,   TEXT("bdcls")  },
			{ EDoomSfx::Itmbk,   TEXT("itmbk")  },
			{ EDoomSfx::Flame,   TEXT("flame")  },
			{ EDoomSfx::Flamst,  TEXT("flamst") },
			{ EDoomSfx::Getpow,  TEXT("getpow") },
			{ EDoomSfx::Bospit,  TEXT("bospit") },
			{ EDoomSfx::Boscub,  TEXT("boscub") },
			{ EDoomSfx::Bossit,  TEXT("bossit") },
			{ EDoomSfx::Bospn,   TEXT("bospn")  },
			{ EDoomSfx::Bosdth,  TEXT("bosdth") },
			{ EDoomSfx::Manatk,  TEXT("manatk") },
			{ EDoomSfx::Mandth,  TEXT("mandth") },
			{ EDoomSfx::Sssit,   TEXT("sssit")  },
			{ EDoomSfx::Ssdth,   TEXT("ssdth")  },
			{ EDoomSfx::Keenpn,  TEXT("keenpn") },
			{ EDoomSfx::Keendt,  TEXT("keendt") },
			{ EDoomSfx::Skeact,  TEXT("skeact") },
			{ EDoomSfx::Skesit,  TEXT("skesit") },
			{ EDoomSfx::Skeatk,  TEXT("skeatk") },
			{ EDoomSfx::Radio,   TEXT("radio")  },
		};

		const FString* Found = SfxNames.Find(SfxId);
		return Found ? *Found : TEXT("none");
	}

	/** Get the original DOOM WAD lump name for a music track (e.g., "e1m1", "runnin") */
	inline FString GetMusicLumpName(EDoomMusic MusicId)
	{
		static const TMap<EDoomMusic, FString> MusicNames = {
			{ EDoomMusic::E1M1,   TEXT("e1m1")   },
			{ EDoomMusic::E1M2,   TEXT("e1m2")   },
			{ EDoomMusic::E1M3,   TEXT("e1m3")   },
			{ EDoomMusic::E1M4,   TEXT("e1m4")   },
			{ EDoomMusic::E1M5,   TEXT("e1m5")   },
			{ EDoomMusic::E1M6,   TEXT("e1m6")   },
			{ EDoomMusic::E1M7,   TEXT("e1m7")   },
			{ EDoomMusic::E1M8,   TEXT("e1m8")   },
			{ EDoomMusic::E1M9,   TEXT("e1m9")   },
			{ EDoomMusic::E2M1,   TEXT("e2m1")   },
			{ EDoomMusic::E2M2,   TEXT("e2m2")   },
			{ EDoomMusic::E2M3,   TEXT("e2m3")   },
			{ EDoomMusic::E2M4,   TEXT("e2m4")   },
			{ EDoomMusic::E2M5,   TEXT("e2m5")   },
			{ EDoomMusic::E2M6,   TEXT("e2m6")   },
			{ EDoomMusic::E2M7,   TEXT("e2m7")   },
			{ EDoomMusic::E2M8,   TEXT("e2m8")   },
			{ EDoomMusic::E2M9,   TEXT("e2m9")   },
			{ EDoomMusic::E3M1,   TEXT("e3m1")   },
			{ EDoomMusic::E3M2,   TEXT("e3m2")   },
			{ EDoomMusic::E3M3,   TEXT("e3m3")   },
			{ EDoomMusic::E3M4,   TEXT("e3m4")   },
			{ EDoomMusic::E3M5,   TEXT("e3m5")   },
			{ EDoomMusic::E3M6,   TEXT("e3m6")   },
			{ EDoomMusic::E3M7,   TEXT("e3m7")   },
			{ EDoomMusic::E3M8,   TEXT("e3m8")   },
			{ EDoomMusic::E3M9,   TEXT("e3m9")   },
			{ EDoomMusic::Inter,  TEXT("inter")  },
			{ EDoomMusic::Intro,  TEXT("intro")  },
			{ EDoomMusic::Bunny,  TEXT("bunny")  },
			{ EDoomMusic::Victor, TEXT("victor") },
			{ EDoomMusic::Introa, TEXT("introa") },
			{ EDoomMusic::Runnin, TEXT("runnin") },
			{ EDoomMusic::Stalks, TEXT("stalks") },
			{ EDoomMusic::Countd, TEXT("countd") },
			{ EDoomMusic::Betwee, TEXT("betwee") },
			{ EDoomMusic::Doom,   TEXT("doom")   },
			{ EDoomMusic::The_Da, TEXT("the_da") },
			{ EDoomMusic::Shawn,  TEXT("shawn")  },
			{ EDoomMusic::Ddtblu, TEXT("ddtblu") },
			{ EDoomMusic::In_Cit, TEXT("in_cit") },
			{ EDoomMusic::Dead,   TEXT("dead")   },
			{ EDoomMusic::Stlks2, TEXT("stlks2") },
			{ EDoomMusic::Theda2, TEXT("theda2") },
			{ EDoomMusic::Doom2,  TEXT("doom2")  },
			{ EDoomMusic::Ddtbl2, TEXT("ddtbl2") },
			{ EDoomMusic::Runni2, TEXT("runni2") },
			{ EDoomMusic::Dead2,  TEXT("dead2")  },
			{ EDoomMusic::Stlks3, TEXT("stlks3") },
			{ EDoomMusic::Romero, TEXT("romero") },
			{ EDoomMusic::Shawn2, TEXT("shawn2") },
			{ EDoomMusic::Messag, TEXT("messag") },
			{ EDoomMusic::Count2, TEXT("count2") },
			{ EDoomMusic::Ddtbl3, TEXT("ddtbl3") },
			{ EDoomMusic::Ampie,  TEXT("ampie")  },
			{ EDoomMusic::Theda3, TEXT("theda3") },
			{ EDoomMusic::Adrian, TEXT("adrian") },
			{ EDoomMusic::Messg2, TEXT("messg2") },
			{ EDoomMusic::Romer2, TEXT("romer2") },
			{ EDoomMusic::Tense,  TEXT("tense")  },
			{ EDoomMusic::Shawn3, TEXT("shawn3") },
			{ EDoomMusic::Openin, TEXT("openin") },
			{ EDoomMusic::Evil,   TEXT("evil")   },
			{ EDoomMusic::Ultima, TEXT("ultima") },
			{ EDoomMusic::Read_M, TEXT("read_m") },
			{ EDoomMusic::Dm2ttl, TEXT("dm2ttl") },
			{ EDoomMusic::Dm2int, TEXT("dm2int") },
		};

		const FString* Found = MusicNames.Find(MusicId);
		return Found ? *Found : TEXT("none");
	}
}
