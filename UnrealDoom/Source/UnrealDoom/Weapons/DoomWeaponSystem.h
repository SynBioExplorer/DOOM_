#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_pspr.c / p_pspr.h - Player weapon sprites and weapon attacks.

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/DoomTypes.h"
#include "DoomWeaponSystem.generated.h"

// =============================================================================
// Weapon sprite slots (from p_pspr.h: ps_weapon, ps_flash, NUMPSPRITES)
// =============================================================================

UENUM(BlueprintType)
enum class EDoomPspriteSlot : uint8
{
	Weapon = 0,     // ps_weapon - The weapon itself
	Flash = 1,      // ps_flash - Muzzle flash overlay
	NumSlots = 2    // NUMPSPRITES
};

// =============================================================================
// Weapon state names - match the S_* state numbers for each weapon
// Matches weaponinfo[] from info.c
// =============================================================================

UENUM(BlueprintType)
enum class EDoomWeaponStateId : uint8
{
	None = 0,
	Ready,
	Up,
	Down,
	Attack,
	Flash
};

// =============================================================================
// FDoomWeaponInfo - Direct port of weaponinfo_t from d_items.h
// =============================================================================

USTRUCT(BlueprintType)
struct FDoomWeaponInfo
{
	GENERATED_BODY()

	/** Ammo type used by this weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDoomAmmo Ammo = EDoomAmmo::NoAmmo;

	/** State IDs for each weapon sprite state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UpState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DownState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ReadyState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AtkState = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FlashState = 0;
};

// =============================================================================
// FDoomPsprite - Direct port of pspdef_t from p_pspr.h
// =============================================================================

USTRUCT(BlueprintType)
struct FDoomPsprite
{
	GENERATED_BODY()

	/** Current state ID (-1 = null, means slot inactive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StateId = -1;

	/** Tics remaining in current state (-1 means never change) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Tics = 0;

	/** Horizontal offset in fixed point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SX = 0;

	/** Vertical offset in fixed point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SY = 0;

	/** Next state to transition to when tics reach 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NextState = 0;
};

// =============================================================================
// Delegates - for the game framework to render sprites and handle effects
// =============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponStateChanged, EDoomWeapon, Weapon, int32, StateId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, EDoomWeapon, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBulletFired, int32, Damage, int32, SpreadAngle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileFired, FName, ProjectileType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSound, FName, SoundName);

// =============================================================================
// UDoomWeaponSystem - Port of p_pspr.c weapon logic
// Attached to the player actor.
// =============================================================================

UCLASS(ClassGroup = (DoomWeapons), meta = (BlueprintSpawnableComponent))
class UNREALDOOM_API UDoomWeaponSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoomWeaponSystem();

	// =========================================================================
	// Player state (ported from player_t)
	// =========================================================================

	/** Currently wielded weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	EDoomWeapon ReadyWeapon = EDoomWeapon::Pistol;

	/** Weapon queued to bring up (NoChange = none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	EDoomWeapon PendingWeapon = EDoomWeapon::NoChange;

	/** Which weapons are owned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	TArray<bool> WeaponOwned;

	/** Current ammo counts, indexed by EDoomAmmo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	TArray<int32> Ammo;

	/** Maximum ammo per type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	TArray<int32> MaxAmmo;

	/** Player health (for weapon drop checks) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	int32 Health = 100;

	/** Whether the attack button is currently held */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	bool bAttackDown = false;

	/** Whether the player is refiring (chaingun/shotgun continuation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	int32 Refire = 0;

	/** Player bob amount (weapon sway) in fixed point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	int32 Bob = 0;

	/** Extra light from muzzle flash (0-2) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom Weapons")
	int32 ExtraLight = 0;

	/** Berserk power active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	bool bBerserk = false;

	/** Player is dead */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	bool bPlayerDead = false;

	/** Game mode (shareware blocks BFG/plasma auto-select) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	EDoomGameMode GameMode = EDoomGameMode::Commercial;

	/** Current level time for weapon bob */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom Weapons")
	int32 LevelTime = 0;

	// =========================================================================
	// Weapon sprites - one per slot (weapon + flash)
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	TArray<FDoomPsprite> Psprites;

	// =========================================================================
	// Weapon info table (from d_items.c weaponinfo[])
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom Weapons")
	TArray<FDoomWeaponInfo> WeaponInfo;

	// =========================================================================
	// Delegates
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Doom Weapons|Events")
	FOnWeaponStateChanged OnWeaponStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Doom Weapons|Events")
	FOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category = "Doom Weapons|Events")
	FOnBulletFired OnBulletFired;

	UPROPERTY(BlueprintAssignable, Category = "Doom Weapons|Events")
	FOnProjectileFired OnProjectileFired;

	UPROPERTY(BlueprintAssignable, Category = "Doom Weapons|Events")
	FOnWeaponSound OnWeaponSound;

	// =========================================================================
	// Core lifecycle
	// =========================================================================

	virtual void BeginPlay() override;

	// =========================================================================
	// Setup and maintenance
	// =========================================================================

	/** P_SetupPsprites: Initialize weapon sprites at level start */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons")
	void SetupPsprites();

	/** P_MovePsprites: Called every tic - advances weapon sprite animations */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons")
	void MovePsprites();

	/** P_SetPsprite: Set a weapon sprite to a new state */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons")
	void SetPsprite(EDoomPspriteSlot Slot, int32 StateId);

	/** P_BringUpWeapon: Start raising the pending weapon */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons")
	void BringUpWeapon();

	/** P_CheckAmmo: Returns true if enough ammo to fire. Auto-switches on empty. */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons")
	bool CheckAmmo();

	/** P_FireWeapon: Begin firing the current weapon */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons")
	void FireWeapon();

	/** P_DropWeapon: Player died - put weapon away */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons")
	void DropWeapon();

	// =========================================================================
	// Weapon action routines - ports from p_pspr.c
	// =========================================================================

	/** A_WeaponReady: Called while weapon is idle - handles fire and weapon change */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_WeaponReady();

	/** A_ReFire: Continuation fire (auto-fire while held) */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_ReFire();

	/** A_CheckReload: Super shotgun reload check */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_CheckReload();

	/** A_Lower: Lower weapon off screen before switching */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_Lower();

	/** A_Raise: Raise weapon onto screen */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_Raise();

	/** A_GunFlash: Trigger muzzle flash state */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_GunFlash();

	/** A_Punch: Fist attack - damage (2-20, *10 with berserk) */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_Punch();

	/** A_Saw: Chainsaw attack - damage (2-20) + hit sound */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_Saw();

	/** A_FirePistol: Single bullet with muzzle flash */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_FirePistol();

	/** A_FireShotgun: 7 pellets spread */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_FireShotgun();

	/** A_FireShotgun2: Super shotgun - 20 pellets double spread */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_FireShotgun2();

	/** A_FireCGun: Chaingun rapid fire */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_FireCGun();

	/** A_FireMissile: Rocket launcher */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_FireMissile();

	/** A_FirePlasma: Plasma rifle with random flash offset */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_FirePlasma();

	/** A_FireBFG: BFG 9000 - consumes 40 cells */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_FireBFG();

	/** A_BFGsound: BFG startup sound */
	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Attacks")
	void A_BFGsound();

	// --- Light level control for muzzle flash ---

	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_Light0();

	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_Light1();

	UFUNCTION(BlueprintCallable, Category = "Doom Weapons|Actions")
	void A_Light2();

	// =========================================================================
	// Helpers
	// =========================================================================

	/** Fire one bullet with standard pistol/chaingun damage formula */
	void GunShot(bool bAccurate);

	/** Reduce ammo for current weapon by Count */
	void ConsumeAmmo(int32 Count);

	/** Get ammo count for the currently ready weapon's ammo type */
	int32 GetCurrentAmmo() const;

	/** Returns pseudo-random 0-255 (shared with AI system) */
	static uint8 P_Random();

	/** Initialize the weapon info table (called in constructor) */
	void InitWeaponInfoTable();

private:
	/** Tick counter for MovePsprites */
	int32 TicCounter = 0;

	/** BFG cell cost */
	static constexpr int32 BFG_CELLS = 40;

	/** Weapon screen position constants */
	static constexpr int32 LOWER_SPEED = FRACUNIT * 6;
	static constexpr int32 RAISE_SPEED = FRACUNIT * 6;
	static constexpr int32 WEAPON_BOTTOM = 128 * FRACUNIT;
	static constexpr int32 WEAPON_TOP = 32 * FRACUNIT;

	/** Current bullet slope for chained shots */
	int32 BulletSlope = 0;
};
