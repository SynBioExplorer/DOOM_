// Copyright (C) 1993-1996 by id Software, Inc.
// UE5 port of p_pspr.c - Weapon sprite animation and weapon attacks.

#include "DoomWeaponSystem.h"
#include "AI/DoomAI.h"  // shares P_Random with AI system

// =============================================================================
// Constructor
// =============================================================================

UDoomWeaponSystem::UDoomWeaponSystem()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize psprite slots
	Psprites.SetNum(static_cast<int32>(EDoomPspriteSlot::NumSlots));

	// Initialize weapon ownership (all weapons start disabled except fist/pistol)
	WeaponOwned.Init(false, static_cast<int32>(EDoomWeapon::NUMWEAPONS));
	if (WeaponOwned.Num() > static_cast<int32>(EDoomWeapon::Fist))
		WeaponOwned[static_cast<int32>(EDoomWeapon::Fist)] = true;
	if (WeaponOwned.Num() > static_cast<int32>(EDoomWeapon::Pistol))
		WeaponOwned[static_cast<int32>(EDoomWeapon::Pistol)] = true;

	// Initialize ammo arrays
	Ammo.Init(0, static_cast<int32>(EDoomAmmo::NUMAMMO));
	MaxAmmo.Init(0, static_cast<int32>(EDoomAmmo::NUMAMMO));

	// Default max ammo from original DOOM (maxammo[] in p_inter.c)
	if (MaxAmmo.Num() >= 4)
	{
		MaxAmmo[0] = 200;  // clip (bullets)
		MaxAmmo[1] = 50;   // shell (shotgun)
		MaxAmmo[2] = 300;  // cell (plasma)
		MaxAmmo[3] = 50;   // missile
	}

	// Starting ammo: 50 bullets for the pistol
	if (Ammo.Num() >= 1)
	{
		Ammo[0] = 50;
	}

	InitWeaponInfoTable();
}

// =============================================================================
// InitWeaponInfoTable - Port of weaponinfo[] from d_items.c
// =============================================================================

void UDoomWeaponSystem::InitWeaponInfoTable()
{
	const int32 NumWeapons = static_cast<int32>(EDoomWeapon::NUMWEAPONS);
	WeaponInfo.SetNum(NumWeapons);

	// Fist: no ammo, melee
	WeaponInfo[static_cast<int32>(EDoomWeapon::Fist)] = {
		EDoomAmmo::NoAmmo, 2, 3, 1, 4, 0
	};

	// Pistol: clip ammo
	WeaponInfo[static_cast<int32>(EDoomWeapon::Pistol)] = {
		EDoomAmmo::Clip, 6, 7, 5, 8, 10
	};

	// Shotgun: shell ammo
	WeaponInfo[static_cast<int32>(EDoomWeapon::Shotgun)] = {
		EDoomAmmo::Shell, 12, 13, 11, 14, 18
	};

	// Chaingun: clip ammo
	WeaponInfo[static_cast<int32>(EDoomWeapon::Chaingun)] = {
		EDoomAmmo::Clip, 20, 21, 19, 22, 26
	};

	// Missile launcher
	WeaponInfo[static_cast<int32>(EDoomWeapon::Missile)] = {
		EDoomAmmo::Missile, 30, 31, 29, 32, 34
	};

	// Plasma rifle
	WeaponInfo[static_cast<int32>(EDoomWeapon::Plasma)] = {
		EDoomAmmo::Cell, 38, 39, 37, 40, 42
	};

	// BFG 9000
	WeaponInfo[static_cast<int32>(EDoomWeapon::BFG)] = {
		EDoomAmmo::Cell, 50, 51, 49, 52, 54
	};

	// Chainsaw
	WeaponInfo[static_cast<int32>(EDoomWeapon::Chainsaw)] = {
		EDoomAmmo::NoAmmo, 58, 59, 57, 60, 0
	};

	// Super shotgun (DOOM II)
	WeaponInfo[static_cast<int32>(EDoomWeapon::SuperShotgun)] = {
		EDoomAmmo::Shell, 70, 71, 69, 72, 76
	};
}

// =============================================================================
// Lifecycle
// =============================================================================

void UDoomWeaponSystem::BeginPlay()
{
	Super::BeginPlay();
	SetupPsprites();
}

// =============================================================================
// Static helpers
// =============================================================================

uint8 UDoomWeaponSystem::P_Random()
{
	// Use AI system's shared RNG
	return UDoomAIComponent::P_Random();
}

int32 UDoomWeaponSystem::GetCurrentAmmo() const
{
	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx < 0 || ReadyIdx >= WeaponInfo.Num())
		return 0;

	const EDoomAmmo AmmoType = WeaponInfo[ReadyIdx].Ammo;
	if (AmmoType == EDoomAmmo::NoAmmo)
		return 999;

	const int32 AmmoIdx = static_cast<int32>(AmmoType);
	if (AmmoIdx < 0 || AmmoIdx >= Ammo.Num())
		return 0;

	return Ammo[AmmoIdx];
}

void UDoomWeaponSystem::ConsumeAmmo(int32 Count)
{
	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx < 0 || ReadyIdx >= WeaponInfo.Num())
		return;

	const EDoomAmmo AmmoType = WeaponInfo[ReadyIdx].Ammo;
	if (AmmoType == EDoomAmmo::NoAmmo)
		return;

	const int32 AmmoIdx = static_cast<int32>(AmmoType);
	if (AmmoIdx < 0 || AmmoIdx >= Ammo.Num())
		return;

	Ammo[AmmoIdx] = FMath::Max(0, Ammo[AmmoIdx] - Count);
}

// =============================================================================
// P_SetPsprite - Set a weapon sprite slot to a new state
// =============================================================================

void UDoomWeaponSystem::SetPsprite(EDoomPspriteSlot Slot, int32 StateId)
{
	const int32 SlotIdx = static_cast<int32>(Slot);
	if (SlotIdx < 0 || SlotIdx >= Psprites.Num())
		return;

	FDoomPsprite& Psp = Psprites[SlotIdx];

	// Loop to handle zero-tic states that chain to next state
	int32 Iterations = 0;
	int32 CurrentState = StateId;

	do
	{
		if (CurrentState == 0)
		{
			// Null state means slot inactive
			Psp.StateId = -1;
			Psp.Tics = 0;
			break;
		}

		Psp.StateId = CurrentState;
		Psp.Tics = 1; // default tics, real impl reads from state table
		Psp.NextState = 0; // real impl reads from state table

		// Broadcast state change for rendering
		OnWeaponStateChanged.Broadcast(ReadyWeapon, CurrentState);

		// If tics > 0 we stop iterating
		if (Psp.Tics != 0)
			break;

		CurrentState = Psp.NextState;

		if (++Iterations > 16)
			break; // safety limit
	}
	while (Psp.Tics == 0);
}

// =============================================================================
// P_BringUpWeapon - Start raising the pending weapon
// =============================================================================

void UDoomWeaponSystem::BringUpWeapon()
{
	if (PendingWeapon == EDoomWeapon::NoChange)
		PendingWeapon = ReadyWeapon;

	if (PendingWeapon == EDoomWeapon::Chainsaw)
		OnWeaponSound.Broadcast(FName("sawup"));

	const int32 PendingIdx = static_cast<int32>(PendingWeapon);
	if (PendingIdx < 0 || PendingIdx >= WeaponInfo.Num())
		return;

	const int32 NewState = WeaponInfo[PendingIdx].UpState;

	PendingWeapon = EDoomWeapon::NoChange;

	// Set weapon to bottom of screen for rise animation
	if (Psprites.Num() > 0)
	{
		Psprites[static_cast<int32>(EDoomPspriteSlot::Weapon)].SY = WEAPON_BOTTOM;
	}

	SetPsprite(EDoomPspriteSlot::Weapon, NewState);
}

// =============================================================================
// P_CheckAmmo - Returns true if enough ammo. Auto-switches on empty.
// =============================================================================

bool UDoomWeaponSystem::CheckAmmo()
{
	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx < 0 || ReadyIdx >= WeaponInfo.Num())
		return false;

	const EDoomAmmo AmmoType = WeaponInfo[ReadyIdx].Ammo;

	// Determine minimum ammo needed for one shot
	int32 Count = 1;
	if (ReadyWeapon == EDoomWeapon::BFG)
		Count = BFG_CELLS;
	else if (ReadyWeapon == EDoomWeapon::SuperShotgun)
		Count = 2;

	// Check if we have enough
	if (AmmoType == EDoomAmmo::NoAmmo)
		return true;

	const int32 AmmoIdx = static_cast<int32>(AmmoType);
	if (AmmoIdx >= 0 && AmmoIdx < Ammo.Num() && Ammo[AmmoIdx] >= Count)
		return true;

	// Out of ammo - pick a weapon to switch to.
	// Preferences match original order from P_CheckAmmo.
	auto CanUse = [this](EDoomWeapon W, EDoomAmmo A, int32 Min) -> bool
	{
		const int32 Idx = static_cast<int32>(W);
		if (Idx < 0 || Idx >= WeaponOwned.Num() || !WeaponOwned[Idx])
			return false;
		const int32 AIdx = static_cast<int32>(A);
		if (AIdx < 0 || AIdx >= Ammo.Num())
			return false;
		return Ammo[AIdx] >= Min;
	};

	do
	{
		if (CanUse(EDoomWeapon::Plasma, EDoomAmmo::Cell, 1) && GameMode != EDoomGameMode::Shareware)
		{
			PendingWeapon = EDoomWeapon::Plasma;
		}
		else if (CanUse(EDoomWeapon::SuperShotgun, EDoomAmmo::Shell, 3) && GameMode == EDoomGameMode::Commercial)
		{
			PendingWeapon = EDoomWeapon::SuperShotgun;
		}
		else if (CanUse(EDoomWeapon::Chaingun, EDoomAmmo::Clip, 1))
		{
			PendingWeapon = EDoomWeapon::Chaingun;
		}
		else if (CanUse(EDoomWeapon::Shotgun, EDoomAmmo::Shell, 1))
		{
			PendingWeapon = EDoomWeapon::Shotgun;
		}
		else if (Ammo.Num() > 0 && Ammo[0] > 0)
		{
			PendingWeapon = EDoomWeapon::Pistol;
		}
		else if (static_cast<int32>(EDoomWeapon::Chainsaw) < WeaponOwned.Num()
			&& WeaponOwned[static_cast<int32>(EDoomWeapon::Chainsaw)])
		{
			PendingWeapon = EDoomWeapon::Chainsaw;
		}
		else if (CanUse(EDoomWeapon::Missile, EDoomAmmo::Missile, 1))
		{
			PendingWeapon = EDoomWeapon::Missile;
		}
		else if (CanUse(EDoomWeapon::BFG, EDoomAmmo::Cell, 41) && GameMode != EDoomGameMode::Shareware)
		{
			PendingWeapon = EDoomWeapon::BFG;
		}
		else
		{
			PendingWeapon = EDoomWeapon::Fist;
		}
	}
	while (PendingWeapon == EDoomWeapon::NoChange);

	// Set downstate to begin weapon swap
	SetPsprite(EDoomPspriteSlot::Weapon, WeaponInfo[ReadyIdx].DownState);
	return false;
}

// =============================================================================
// P_FireWeapon
// =============================================================================

void UDoomWeaponSystem::FireWeapon()
{
	if (!CheckAmmo())
		return;

	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx < 0 || ReadyIdx >= WeaponInfo.Num())
		return;

	const int32 NewState = WeaponInfo[ReadyIdx].AtkState;
	SetPsprite(EDoomPspriteSlot::Weapon, NewState);
	OnWeaponFired.Broadcast(ReadyWeapon);
}

// =============================================================================
// P_DropWeapon
// =============================================================================

void UDoomWeaponSystem::DropWeapon()
{
	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx < 0 || ReadyIdx >= WeaponInfo.Num())
		return;

	SetPsprite(EDoomPspriteSlot::Weapon, WeaponInfo[ReadyIdx].DownState);
}

// =============================================================================
// P_SetupPsprites
// =============================================================================

void UDoomWeaponSystem::SetupPsprites()
{
	// Clear all psprites
	for (int32 i = 0; i < Psprites.Num(); i++)
	{
		Psprites[i].StateId = -1;
		Psprites[i].Tics = 0;
		Psprites[i].SX = FRACUNIT;
		Psprites[i].SY = WEAPON_TOP;
	}

	PendingWeapon = ReadyWeapon;
	BringUpWeapon();
}

// =============================================================================
// P_MovePsprites - Called every tic
// =============================================================================

void UDoomWeaponSystem::MovePsprites()
{
	LevelTime++;

	for (int32 i = 0; i < Psprites.Num(); i++)
	{
		FDoomPsprite& Psp = Psprites[i];

		if (Psp.StateId <= 0)
			continue;  // slot inactive

		// Tics == -1 means never change (holds indefinitely)
		if (Psp.Tics != -1)
		{
			Psp.Tics--;
			if (Psp.Tics <= 0)
			{
				SetPsprite(static_cast<EDoomPspriteSlot>(i), Psp.NextState);
			}
		}
	}

	// Flash sprite follows the weapon sprite position (line 875-876 original)
	if (Psprites.Num() > static_cast<int32>(EDoomPspriteSlot::Flash))
	{
		const int32 W = static_cast<int32>(EDoomPspriteSlot::Weapon);
		const int32 F = static_cast<int32>(EDoomPspriteSlot::Flash);
		Psprites[F].SX = Psprites[W].SX;
		Psprites[F].SY = Psprites[W].SY;
	}
}

// =============================================================================
// A_WeaponReady - Handles fire input and weapon change
// =============================================================================

void UDoomWeaponSystem::A_WeaponReady()
{
	// Chainsaw idle sound
	if (ReadyWeapon == EDoomWeapon::Chainsaw)
	{
		OnWeaponSound.Broadcast(FName("sawidl"));
	}

	// Check for weapon change or player dead
	if (PendingWeapon != EDoomWeapon::NoChange || Health == 0)
	{
		const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
		if (ReadyIdx >= 0 && ReadyIdx < WeaponInfo.Num())
		{
			SetPsprite(EDoomPspriteSlot::Weapon, WeaponInfo[ReadyIdx].DownState);
		}
		return;
	}

	// Check for fire button
	if (bAttackDown)
	{
		// Missile/BFG don't autofire - require press-release-press
		const bool bAutoFire = (ReadyWeapon != EDoomWeapon::Missile && ReadyWeapon != EDoomWeapon::BFG);

		if (!bAttackDown || bAutoFire)
		{
			FireWeapon();
			return;
		}
	}

	// Bob the weapon based on movement
	const int32 Angle = (128 * LevelTime) & 8191; // FINEMASK
	const int32 WeaponSlot = static_cast<int32>(EDoomPspriteSlot::Weapon);
	if (Psprites.IsValidIndex(WeaponSlot))
	{
		// Simplified bob: sine wave based on level time
		const float Rad = (Angle * 2.0f * PI) / 8192.0f;
		const float BobScale = static_cast<float>(Bob) / FRACUNIT;
		Psprites[WeaponSlot].SX = FRACUNIT + static_cast<int32>(BobScale * FMath::Cos(Rad) * FRACUNIT);
		Psprites[WeaponSlot].SY = WEAPON_TOP + static_cast<int32>(BobScale * FMath::Sin(Rad) * FRACUNIT);
	}
}

// =============================================================================
// A_ReFire
// =============================================================================

void UDoomWeaponSystem::A_ReFire()
{
	if (bAttackDown && PendingWeapon == EDoomWeapon::NoChange && Health > 0)
	{
		Refire++;
		FireWeapon();
	}
	else
	{
		Refire = 0;
		CheckAmmo();
	}
}

// =============================================================================
// A_CheckReload
// =============================================================================

void UDoomWeaponSystem::A_CheckReload()
{
	CheckAmmo();
}

// =============================================================================
// A_Lower - Lower weapon off screen
// =============================================================================

void UDoomWeaponSystem::A_Lower()
{
	const int32 WeaponSlot = static_cast<int32>(EDoomPspriteSlot::Weapon);
	if (!Psprites.IsValidIndex(WeaponSlot))
		return;

	Psprites[WeaponSlot].SY += LOWER_SPEED;

	if (Psprites[WeaponSlot].SY < WEAPON_BOTTOM)
		return;

	if (bPlayerDead)
	{
		Psprites[WeaponSlot].SY = WEAPON_BOTTOM;
		return;
	}

	if (Health == 0)
	{
		SetPsprite(EDoomPspriteSlot::Weapon, 0);
		return;
	}

	ReadyWeapon = PendingWeapon;
	BringUpWeapon();
}

// =============================================================================
// A_Raise
// =============================================================================

void UDoomWeaponSystem::A_Raise()
{
	const int32 WeaponSlot = static_cast<int32>(EDoomPspriteSlot::Weapon);
	if (!Psprites.IsValidIndex(WeaponSlot))
		return;

	Psprites[WeaponSlot].SY -= RAISE_SPEED;

	if (Psprites[WeaponSlot].SY > WEAPON_TOP)
		return;

	Psprites[WeaponSlot].SY = WEAPON_TOP;

	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx >= 0 && ReadyIdx < WeaponInfo.Num())
	{
		SetPsprite(EDoomPspriteSlot::Weapon, WeaponInfo[ReadyIdx].ReadyState);
	}
}

// =============================================================================
// A_GunFlash
// =============================================================================

void UDoomWeaponSystem::A_GunFlash()
{
	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx >= 0 && ReadyIdx < WeaponInfo.Num())
	{
		SetPsprite(EDoomPspriteSlot::Flash, WeaponInfo[ReadyIdx].FlashState);
	}
}

// =============================================================================
// Weapon attacks
// =============================================================================

void UDoomWeaponSystem::A_Punch()
{
	// damage = (P_Random()%10+1)<<1 -> range [2, 20]
	int32 Damage = (P_Random() % 10 + 1) << 1;

	// Berserk multiplier
	if (bBerserk)
		Damage *= 10;

	// Spread: angle += (P_Random()-P_Random()) << 18
	const int32 Spread = (static_cast<int32>(P_Random()) - static_cast<int32>(P_Random())) << 18;

	OnBulletFired.Broadcast(Damage, Spread);
	OnWeaponSound.Broadcast(FName("punch"));
}

void UDoomWeaponSystem::A_Saw()
{
	// damage = 2*(P_Random()%10+1) -> range [2, 20]
	const int32 Damage = 2 * (P_Random() % 10 + 1);
	const int32 Spread = (static_cast<int32>(P_Random()) - static_cast<int32>(P_Random())) << 18;

	OnBulletFired.Broadcast(Damage, Spread);
	OnWeaponSound.Broadcast(FName("sawhit"));
}

void UDoomWeaponSystem::A_FirePistol()
{
	OnWeaponSound.Broadcast(FName("pistol"));
	ConsumeAmmo(1);
	A_GunFlash();
	GunShot(Refire == 0); // accurate on first shot
}

void UDoomWeaponSystem::A_FireShotgun()
{
	OnWeaponSound.Broadcast(FName("shotgn"));
	ConsumeAmmo(1);
	A_GunFlash();

	// 7 pellets
	for (int32 i = 0; i < 7; i++)
	{
		GunShot(false);
	}
}

void UDoomWeaponSystem::A_FireShotgun2()
{
	OnWeaponSound.Broadcast(FName("dshtgn"));
	ConsumeAmmo(2);
	A_GunFlash();

	// 20 pellets with wider spread
	for (int32 i = 0; i < 20; i++)
	{
		const int32 Damage = 5 * (P_Random() % 3 + 1);
		const int32 Spread = (static_cast<int32>(P_Random()) - static_cast<int32>(P_Random())) << 19;
		OnBulletFired.Broadcast(Damage, Spread);
	}
}

void UDoomWeaponSystem::A_FireCGun()
{
	if (GetCurrentAmmo() <= 0)
		return;

	OnWeaponSound.Broadcast(FName("pistol"));
	ConsumeAmmo(1);
	A_GunFlash();
	GunShot(Refire == 0);
}

void UDoomWeaponSystem::A_FireMissile()
{
	ConsumeAmmo(1);
	OnProjectileFired.Broadcast(FName("Rocket"));
}

void UDoomWeaponSystem::A_FirePlasma()
{
	ConsumeAmmo(1);

	// Random flash frame
	const int32 ReadyIdx = static_cast<int32>(ReadyWeapon);
	if (ReadyIdx >= 0 && ReadyIdx < WeaponInfo.Num())
	{
		SetPsprite(EDoomPspriteSlot::Flash,
			WeaponInfo[ReadyIdx].FlashState + (P_Random() & 1));
	}

	OnProjectileFired.Broadcast(FName("Plasma"));
}

void UDoomWeaponSystem::A_FireBFG()
{
	ConsumeAmmo(BFG_CELLS);
	OnProjectileFired.Broadcast(FName("BFG"));
}

void UDoomWeaponSystem::A_BFGsound()
{
	OnWeaponSound.Broadcast(FName("bfg"));
}

// =============================================================================
// Light level control
// =============================================================================

void UDoomWeaponSystem::A_Light0() { ExtraLight = 0; }
void UDoomWeaponSystem::A_Light1() { ExtraLight = 1; }
void UDoomWeaponSystem::A_Light2() { ExtraLight = 2; }

// =============================================================================
// GunShot - Fire a single bullet
// =============================================================================

void UDoomWeaponSystem::GunShot(bool bAccurate)
{
	// Standard DOOM pistol/chaingun damage: 5*(P_Random()%3+1) = 5, 10, or 15
	const int32 Damage = 5 * (P_Random() % 3 + 1);

	int32 Spread = 0;
	if (!bAccurate)
	{
		Spread = (static_cast<int32>(P_Random()) - static_cast<int32>(P_Random())) << 18;
	}

	OnBulletFired.Broadcast(Damage, Spread);
}
