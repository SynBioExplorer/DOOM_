#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_enemy.c - Enemy AI, thinking, and action pointer functions.

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/DoomTypes.h"
#include "Core/DoomMath.h"
#include "DoomAI.generated.h"

// Forward declarations
class ADoomEntity;
class UDoomSightChecker;
class UDoomCombat;

// =============================================================================
// Direction enumeration - 8-directional movement for AI pathfinding
// =============================================================================

UENUM(BlueprintType)
enum class EDoomDirection : uint8
{
	East = 0,       // DI_EAST
	NorthEast = 1,  // DI_NORTHEAST
	North = 2,      // DI_NORTH
	NorthWest = 3,  // DI_NORTHWEST
	West = 4,       // DI_WEST
	SouthWest = 5,  // DI_SOUTHWEST
	South = 6,      // DI_SOUTH
	SouthEast = 7,  // DI_SOUTHEAST
	NoDir = 8,      // DI_NODIR
	NumDirs = 9     // NUMDIRS
};

// =============================================================================
// Monster type enumeration - for AI behavior branching
// =============================================================================

UENUM(BlueprintType)
enum class EDoomMonsterType : uint8
{
	Possessed = 0,      // MT_POSSESSED - Zombieman
	ShotGuy = 1,        // MT_SHOTGUY - Shotgun guy
	ChainGuy = 2,       // MT_CHAINGUY - Chaingunner (Heavy weapon dude)
	Troop = 3,          // MT_TROOP - Imp
	Sergeant = 4,       // MT_SERGEANT - Demon (Pinky)
	Shadows = 5,        // MT_SHADOWS - Spectre
	Head = 6,           // MT_HEAD - Cacodemon
	Bruiser = 7,        // MT_BRUISER - Baron of Hell
	Knight = 8,         // MT_KNIGHT - Hell Knight
	Skull = 9,          // MT_SKULL - Lost Soul
	Spider = 10,        // MT_SPIDER - Spider Mastermind
	Baby = 11,          // MT_BABY - Arachnotron
	Cyborg = 12,        // MT_CYBORG - Cyberdemon
	Pain = 13,          // MT_PAIN - Pain Elemental
	Undead = 14,        // MT_UNDEAD - Revenant
	Fatso = 15,         // MT_FATSO - Mancubus
	Vile = 16,          // MT_VILE - Arch-Vile
	Keen = 17,          // MT_KEEN - Commander Keen
	BossBrain = 18,     // MT_BOSSBRAIN - Icon of Sin brain
	BossEye = 19,       // MT_BOSSSPIT - Icon of Sin eye (shooter)
	BossTarget = 20,    // MT_BOSSTARGET - Spawn spot
	SpawnShot = 21,     // MT_SPAWNSHOT - Cube projectile
	Player = 22,        // MT_PLAYER
	Barrel = 23,        // MT_BARREL - Explosive barrel
	Max = 24
};

// =============================================================================
// AI state for the component's current behavior
// =============================================================================

UENUM(BlueprintType)
enum class EDoomAIState : uint8
{
	Idle = 0,       // Standing, waiting (A_Look state)
	Chase = 1,      // Pursuing target (A_Chase state)
	Attack = 2,     // Performing an attack
	Pain = 3,       // Flinching from damage
	Death = 4,      // Dying
	Raise = 5,      // Being resurrected by Archvile
	Special = 6     // Special behavior (Icon of Sin, etc.)
};

// =============================================================================
// Projectile types that can be spawned
// =============================================================================

UENUM(BlueprintType)
enum class EDoomProjectileType : uint8
{
	ImpFireball = 0,    // MT_TROOPSHOT
	CacoBall = 1,       // MT_HEADSHOT
	BaronBall = 2,      // MT_BRUISERSHOT
	Rocket = 3,         // MT_ROCKET
	PlasmaBall = 4,     // MT_ARACHPLAZ
	FatShot = 5,        // MT_FATSHOT
	TracerMissile = 6,  // MT_TRACER - Revenant homing missile
	SpawnShot = 7,      // MT_SPAWNSHOT - Icon of Sin cube
	Max = 8
};

// =============================================================================
// Delegates for events that the game framework must handle
// =============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDoomAIStateChanged, EDoomAIState, OldState, EDoomAIState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoomAIAttack, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDoomAISpawnProjectile, EDoomProjectileType, ProjectileType, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDoomAIDamage, AActor*, Target, int32, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoomAIPlaySound, FName, SoundName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoomAISpecialEvent, FName, EventName);

// =============================================================================
// UDoomAIComponent - Faithful port of DOOM enemy AI (p_enemy.c)
// =============================================================================

UCLASS(ClassGroup = (DoomAI), meta = (BlueprintSpawnableComponent))
class UNREALDOOM_API UDoomAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoomAIComponent();

	// =========================================================================
	// Configuration - set per monster type
	// =========================================================================

	/** The type of monster this AI controls */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	EDoomMonsterType MonsterType = EDoomMonsterType::Possessed;

	/** Movement speed in DOOM fixed-point units per tic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	int32 Speed = 8;

	/** Monster radius in DOOM fixed-point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	fixed_t Radius = 20 * FRACUNIT;

	/** Monster height in DOOM fixed-point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	fixed_t Height = 56 * FRACUNIT;

	/** Monster mass (affects knockback) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	int32 Mass = 100;

	/** Whether this monster has a melee attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	bool bHasMeleeAttack = false;

	/** Whether this monster has a missile/ranged attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	bool bHasMissileAttack = false;

	/** Projectile type for ranged attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	EDoomProjectileType ProjectileType = EDoomProjectileType::ImpFireball;

	/** See sound name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	FName SeeSound;

	/** Attack sound name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	FName AttackSound;

	/** Pain sound name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	FName PainSound;

	/** Death sound name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	FName DeathSound;

	/** Active/idle sound name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	FName ActiveSound;

	/** Can float (Cacodemon, Pain Elemental, Lost Soul) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Config")
	bool bCanFloat = false;

	// =========================================================================
	// Runtime state
	// =========================================================================

	/** Current AI behavioral state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	EDoomAIState AIState = EDoomAIState::Idle;

	/** Current movement direction (8-directional) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	EDoomDirection MoveDir = EDoomDirection::NoDir;

	/** Steps before direction change (countdown) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	int32 MoveCount = 0;

	/** Current target actor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	TWeakObjectPtr<AActor> Target;

	/** Threshold for target switching (follow current target for N tics) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	int32 Threshold = 0;

	/** Reaction time countdown before attacking */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	int32 ReactionTime = 8;

	/** Last player index looked at (round-robin for multiplayer) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	int32 LastLook = 0;

	/** Monster health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|State")
	int32 Health = 20;

	/** MF_ flags word, matches original DOOM mobjflag_t bits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|State")
	int32 Flags = 0;

	/** Current facing angle (DOOM binary angle) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	angle_t FacingAngle = 0;

	/** Whether floating movement is valid at current position */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom AI|State")
	bool bFloatOk = false;

	// =========================================================================
	// Game state references
	// =========================================================================

	/** Current skill level (affects attack frequency) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Game")
	EDoomSkill GameSkill = EDoomSkill::Medium;

	/** Whether this is a network game */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Game")
	bool bNetGame = false;

	/** Whether fast monsters are enabled (-fast parameter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Game")
	bool bFastParm = false;

	/** All player actors in the game */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Game")
	TArray<AActor*> PlayerActors;

	/** Sight checker subsystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Game")
	UDoomSightChecker* SightChecker = nullptr;

	/** Combat subsystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom AI|Game")
	UDoomCombat* CombatSystem = nullptr;

	// =========================================================================
	// Delegates / Events
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Doom AI|Events")
	FOnDoomAIStateChanged OnAIStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Doom AI|Events")
	FOnDoomAIAttack OnAttack;

	UPROPERTY(BlueprintAssignable, Category = "Doom AI|Events")
	FOnDoomAISpawnProjectile OnSpawnProjectile;

	UPROPERTY(BlueprintAssignable, Category = "Doom AI|Events")
	FOnDoomAIDamage OnDamage;

	UPROPERTY(BlueprintAssignable, Category = "Doom AI|Events")
	FOnDoomAIPlaySound OnPlaySound;

	UPROPERTY(BlueprintAssignable, Category = "Doom AI|Events")
	FOnDoomAISpecialEvent OnSpecialEvent;

	// =========================================================================
	// Core lifecycle
	// =========================================================================

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// =========================================================================
	// Action routines - direct ports from p_enemy.c
	// Each is callable as a UFUNCTION for Blueprint use.
	// =========================================================================

	// --- State behaviors ---

	/** A_Look: Idle state, search for player via sight and sound.
	 *  Stays in idle until a player is sighted or heard. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Look();

	/** A_Chase: Pursue target with movement, melee, and missile decisions.
	 *  Faithfully ports the original chase logic including direction turning,
	 *  melee/missile checks, target switching in netgames, and active sounds. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Chase();

	/** A_FaceTarget: Turn to face current target.
	 *  Includes random angle jitter when target has MF_SHADOW (spectres). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_FaceTarget();

	// --- Individual monster attacks ---

	/** A_PosAttack: Zombieman attack - single hitscan bullet.
	 *  Damage: ((P_Random()%5)+1)*3 = 3-15 */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_PosAttack();

	/** A_SPosAttack: Shotgun Guy attack - 3 hitscan pellets.
	 *  Each pellet: ((P_Random()%5)+1)*3 = 3-15 */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SPosAttack();

	/** A_CPosAttack: Chaingunner attack - single hitscan bullet per frame.
	 *  Same damage as A_PosAttack per bullet but fires continuously. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_CPosAttack();

	/** A_CPosRefire: Chaingunner refire check.
	 *  Keeps firing unless target lost sight or random chance. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_CPosRefire();

	/** A_SpidRefire: Spider Mastermind refire check (lower stop chance than chaingunner). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SpidRefire();

	/** A_TroopAttack: Imp attack - melee claw (3-24) or fireball missile. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_TroopAttack();

	/** A_SargAttack: Demon bite - melee only ((P_Random()%10)+1)*4 = 4-40. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SargAttack();

	/** A_HeadAttack: Cacodemon attack - melee (10-60) or lightning ball. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_HeadAttack();

	/** A_BruisAttack: Baron of Hell attack - melee claw (10-80) or green fireball. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_BruisAttack();

	/** A_CyberAttack: Cyberdemon rocket launch. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_CyberAttack();

	/** A_SpidAttack: Spider Mastermind super chaingun (same as A_CPosAttack). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SpidAttack();

	/** A_BspiAttack: Arachnotron plasma rifle. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_BspiAttack();

	/** A_SkullAttack: Lost Soul charge - fly at player like a missile.
	 *  Speed = 20*FRACUNIT. Sets MF_SKULLFLY flag. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SkullAttack();

	// --- Revenant ---

	/** A_SkelMissile: Revenant homing missile launch. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SkelMissile();

	/** A_SkelWhoosh: Revenant fist whoosh sound. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SkelWhoosh();

	/** A_SkelFist: Revenant punch - ((P_Random()%10)+1)*6 = 6-60. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_SkelFist();

	/** A_Tracer: Homing missile guidance (called every 4th tic). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_Tracer();

	// --- Mancubus ---

	/** A_FatRaise: Mancubus raise sound before attack. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_FatRaise();

	/** A_FatAttack1: Mancubus fireball volley 1 (two fireballs, spread right). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_FatAttack1();

	/** A_FatAttack2: Mancubus fireball volley 2 (two fireballs, spread left). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_FatAttack2();

	/** A_FatAttack3: Mancubus fireball volley 3 (two fireballs, slight spread). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_FatAttack3();

	// --- Archvile ---

	/** A_VileChase: Archvile chase that also searches for corpses to resurrect. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_VileChase();

	/** A_VileStart: Archvile attack windup sound. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_VileStart();

	/** A_VileTarget: Spawn hellfire at target's feet. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_VileTarget();

	/** A_VileAttack: Archvile flame attack - 20 direct + 70 radius.
	 *  Launches target into the air. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_VileAttack();

	/** A_Fire: Keep fire sprite in front of Archvile's target. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_Fire();

	/** A_StartFire: Start Archvile fire with sound. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_StartFire();

	/** A_FireCrackle: Archvile fire crackling sound. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_FireCrackle();

	// --- Pain Elemental ---

	/** A_PainAttack: Spawn a Lost Soul and launch it at the target. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_PainAttack();

	/** A_PainDie: Death - spawns 3 Lost Souls at 90/180/270 degrees. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Attacks")
	void A_PainDie();

	// --- Commander Keen ---

	/** A_KeenDie: DOOM II MAP32 special - when all Keens die, open tag 666 door. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_KeenDie();

	// --- Icon of Sin ---

	/** A_BrainAwake: Find all boss target spots and play awakening sound. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_BrainAwake();

	/** A_BrainSpit: Shoot a spawn cube at the next target spot. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_BrainSpit();

	/** A_BrainDie: End the level when brain is destroyed. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_BrainDie();

	/** A_BrainExplode: Spawn random explosion around the brain. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_BrainExplode();

	/** A_SpawnFly: Spawn cube reaches destination, spawns random monster. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_SpawnFly();

	/** A_SpawnSound: Travelling cube sound + SpawnFly check. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_SpawnSound();

	/** A_BrainScream: Boss brain death explosion cascade. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_BrainScream();

	/** A_BrainPain: Boss brain pain sound. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_BrainPain();

	// --- Generic actions ---

	/** A_Fall: Monster dies, becomes non-solid (can be walked over). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Fall();

	/** A_Explode: Radius attack with 128 damage (rockets, barrels). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Explode();

	/** A_Scream: Play death sound with random variation for zombie/imp types. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Scream();

	/** A_XScream: Extra-violent death scream (gib sound). */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_XScream();

	/** A_Pain: Play pain sound. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Pain();

	/** A_BossDeath: Special level-end triggers when bosses die. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Special")
	void A_BossDeath();

	/** A_Hoof: Cyberdemon hoof stomp sound + chase. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Hoof();

	/** A_Metal: Spider Mastermind metal walk sound + chase. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_Metal();

	/** A_BabyMetal: Arachnotron walk sound + chase. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Actions")
	void A_BabyMetal();

	// =========================================================================
	// Movement system - ports from p_enemy.c
	// =========================================================================

	/** P_Move: Try to move in current direction.
	 *  Returns false if blocked. Handles floating, door opening. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Movement")
	bool P_Move();

	/** P_TryWalk: Attempt to walk in current direction.
	 *  Resets movecount on success. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Movement")
	bool P_TryWalk();

	/** P_NewChaseDir: 8-directional pathfinding toward target.
	 *  Tries direct route, then axis-aligned, then random directions. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Movement")
	void P_NewChaseDir();

	/** P_LookForPlayers: Scan for visible players.
	 *  @param bAllAround - if false, only check 180 degrees in front.
	 *  @return true if a player was targeted. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Movement")
	bool P_LookForPlayers(bool bAllAround);

	/** P_CheckMeleeRange: Is the target within melee striking distance?
	 *  Range = MELEERANGE (64) - 20 + target radius. Requires line of sight. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Movement")
	bool P_CheckMeleeRange() const;

	/** P_CheckMissileRange: Should this monster fire a missile?
	 *  Considers distance, reaction time, monster type, and random chance. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Movement")
	bool P_CheckMissileRange();

	// =========================================================================
	// Sound propagation - port of P_RecursiveSound / P_NoiseAlert
	// =========================================================================

	/** Propagate noise alert through connected sectors.
	 *  Wakes up all monsters that can hear the sound through open passages.
	 *  Sound-blocking lines limit propagation depth. */
	UFUNCTION(BlueprintCallable, Category = "Doom AI|Sound")
	static void NoiseAlert(AActor* Target, AActor* Emitter);

	// =========================================================================
	// Internal helpers - spawn helper for Pain Elemental
	// =========================================================================

	/** Spawn a Lost Soul and launch it in the given direction.
	 *  Enforces the 20-skull limit from the original game. */
	void PainShootSkull(angle_t Angle);

	// =========================================================================
	// Utility
	// =========================================================================

	/** Get the owner actor's world position as a DOOM fixed-point coordinate */
	void GetOwnerDoomPosition(fixed_t& OutX, fixed_t& OutY, fixed_t& OutZ) const;

	/** Get an actor's position as DOOM fixed-point */
	static void GetActorDoomPosition(const AActor* Actor, fixed_t& OutX, fixed_t& OutY, fixed_t& OutZ);

	/** Compute approximate distance (DOOM-style: max + min/2) */
	static fixed_t AproxDistance(fixed_t DX, fixed_t DY);

	/** Compute angle from (x1,y1) to (x2,y2) using DOOM's BAM system */
	static angle_t PointToAngle2(fixed_t X1, fixed_t Y1, fixed_t X2, fixed_t Y2);

	/** Get a DOOM-style pseudo-random number (0-255) */
	static uint8 P_Random();

	/** Change AI state with delegate broadcast */
	void SetAIState(EDoomAIState NewState);

	/** Check if target actor is still valid and alive */
	bool IsTargetValid() const;

	/** Get target actor's health (returns 0 if invalid) */
	int32 GetTargetHealth() const;

	/** Get target actor's flags (returns 0 if invalid) */
	int32 GetTargetFlags() const;

private:

	// =========================================================================
	// Internal state
	// =========================================================================

	/** Tic counter for game time tracking */
	int32 GameTic = 0;

	/** Accumulated time for tic simulation */
	float TicAccumulator = 0.0f;

	/** DOOM tic duration in seconds */
	static constexpr float TIC_DURATION = 1.0f / 35.0f;

	// Direction speed lookup tables - exact DOOM values
	static const fixed_t XSpeed[8];
	static const fixed_t YSpeed[8];

	// Direction opposites lookup
	static const EDoomDirection Opposite[9];

	// Diagonal directions lookup
	static const EDoomDirection Diags[4];

	// --- Icon of Sin state ---

	/** Brain target spots for Icon of Sin */
	UPROPERTY()
	TArray<AActor*> BrainTargets;

	/** Current brain target index */
	int32 BrainTargetOn = 0;

	/** Easy mode toggle for brain spit frequency */
	int32 BrainEasy = 0;

	/** Maximum Lost Souls allowed on the level */
	static constexpr int32 MAX_SKULLS_ON_LEVEL = 20;

	/** Skull attack speed (20 * FRACUNIT) */
	static constexpr fixed_t SKULL_SPEED = 20 * FRACUNIT;

	/** Mancubus fireball spread angle (ANG90/8) */
	static constexpr angle_t FAT_SPREAD = ANG90 / 8;

	/** Revenant tracer turn angle */
	static constexpr int32 TRACE_ANGLE = 0xc000000;

	/** Follow threshold before retargeting */
	static constexpr int32 BASE_THRESHOLD = 100;

	// DOOM play constants
	static constexpr fixed_t MELEE_RANGE = 64 * FRACUNIT;
	static constexpr fixed_t MISSILE_RANGE = 32 * 64 * FRACUNIT;
	static constexpr fixed_t FLOAT_SPEED = 4 * FRACUNIT;

	/** Random number table index */
	static int32 RndIndex;

	/** The DOOM random number table (256 entries) */
	static const uint8 RndTable[256];
};
