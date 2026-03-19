#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomHUDWidget - Full recreation of the DOOM status bar using UMG.
// Displays health, armor, ammo, key cards, weapon slots, the DOOM face,
// HUD messages, and screen flash overlays for damage/pickups.
//
// Equivalent to st_stuff.c / st_lib.c.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/DoomTypes.h"
#include "DoomHUD.generated.h"

class UTextBlock;
class UImage;
class UOverlay;

// Forward declaration - the player component that provides HUD data.
// This will be a game-specific component holding health, armor, ammo, etc.
class ADoomPlayerPawn;

/**
 * UDoomHUDWidget
 *
 * UMG widget that recreates the classic DOOM status bar (ST_BAR).
 * Bind this widget to a Widget Blueprint that contains the named
 * sub-widgets (using meta=(BindWidget) for compile-time checking).
 *
 * Layout matches the original 320x32 status bar:
 * - Left: Current ammo (large number)
 * - Center-left: Health %
 * - Center: DOOM face (status indicator)
 * - Center-right: Armor %
 * - Bottom row: Arms (weapon availability), Key cards/skulls
 *
 * Also provides:
 * - HUD message display (top of screen, timed fade)
 * - Screen flash overlay for damage (red) and pickups (gold)
 */
UCLASS()
class UNREALDOOM_API UDoomHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDoomHUDWidget(const FObjectInitializer& ObjectInitializer);

	// =========================================================================
	// Player binding
	// =========================================================================

	/**
	 * Bind the HUD to a player pawn to read health, armor, ammo, etc.
	 * Call this after the widget is constructed and the player is available.
	 *
	 * @param InPlayerPawn - The player pawn to display stats for
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void BindToPlayer(ADoomPlayerPawn* InPlayerPawn);

	// =========================================================================
	// Manual stat setters (for use without a bound player component)
	// =========================================================================

	/** Set the health display value */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void SetHealth(int32 Value);

	/** Set the armor display value */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void SetArmor(int32 Value);

	/** Set the current ammo display value */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void SetAmmo(int32 Value);

	/** Set key card/skull visibility */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void SetKeyCard(EDoomCard Card, bool bHasKey);

	/** Set weapon slot availability */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void SetWeaponAvailable(int32 SlotIndex, bool bAvailable);

	/** Set the face/status image index */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void SetFaceIndex(int32 FaceIndex);

	// =========================================================================
	// HUD message system
	// =========================================================================

	/**
	 * Show a HUD message at the top of the screen.
	 * Messages auto-fade after MessageDisplayTime seconds.
	 *
	 * @param Message - The message text to display
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void ShowMessage(const FString& Message);

	// =========================================================================
	// Screen flash
	// =========================================================================

	/**
	 * Trigger a damage flash (red overlay).
	 * @param Intensity - Flash intensity (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void TriggerDamageFlash(float Intensity);

	/**
	 * Trigger a pickup flash (gold overlay).
	 * @param Intensity - Flash intensity (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|HUD")
	void TriggerPickupFlash(float Intensity);

protected:
	// =========================================================================
	// UUserWidget overrides
	// =========================================================================

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// =========================================================================
	// Bound widgets - status bar values
	// =========================================================================

	/** Large current ammo counter (left side of status bar) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentAmmoText;

	/** Health percentage display */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	/** Armor percentage display */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ArmorText;

	/** Secondary ammo display (total for current weapon type) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AmmoText;

	// =========================================================================
	// Bound widgets - key cards / skulls (6 indicators)
	// =========================================================================

	/** Blue key card indicator */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> KeyCard_Blue;

	/** Yellow key card indicator */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> KeyCard_Yellow;

	/** Red key card indicator */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> KeyCard_Red;

	/** Blue skull key indicator */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> KeySkull_Blue;

	/** Yellow skull key indicator */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> KeySkull_Yellow;

	/** Red skull key indicator */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> KeySkull_Red;

	// =========================================================================
	// Bound widgets - Arms (weapon slot indicators, slots 2-8)
	// =========================================================================

	/** Weapon slot 1 indicator (Fist/Chainsaw) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Arms_Slot1;

	/** Weapon slot 2 indicator (Pistol) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Arms_Slot2;

	/** Weapon slot 3 indicator (Shotgun/SSG) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Arms_Slot3;

	/** Weapon slot 4 indicator (Chaingun) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Arms_Slot4;

	/** Weapon slot 5 indicator (Rocket Launcher) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Arms_Slot5;

	/** Weapon slot 6 indicator (Plasma Rifle) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Arms_Slot6;

	/** Weapon slot 7 indicator (BFG9000) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Arms_Slot7;

	// =========================================================================
	// Bound widgets - DOOM face
	// =========================================================================

	/** The DOOM guy face status indicator */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> FaceWidget;

	// =========================================================================
	// Bound widgets - HUD message and flash overlay
	// =========================================================================

	/** HUD message text (top of screen) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	/** Full-screen color overlay for damage/pickup flash */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ScreenFlashOverlay;

	// =========================================================================
	// State
	// =========================================================================

	/** The player pawn we are reading stats from */
	UPROPERTY()
	TObjectPtr<ADoomPlayerPawn> BoundPlayerPawn;

	/** Current cached values for change detection */
	int32 CachedHealth = -1;
	int32 CachedArmor = -1;
	int32 CachedAmmo = -1;

	/** Key card states */
	bool bKeyCards[6] = { false, false, false, false, false, false };

	/** Weapon availability states */
	bool bWeaponSlots[7] = { false, false, false, false, false, false, false };

	// =========================================================================
	// HUD message timer
	// =========================================================================

	/** How long to display HUD messages (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|HUD|Settings")
	float MessageDisplayTime = 4.0f;

	/** Remaining time for the current message */
	float MessageTimer = 0.0f;

	// =========================================================================
	// Screen flash state
	// =========================================================================

	/** Current flash intensity (decays to 0) */
	float FlashIntensity = 0.0f;

	/** Current flash color */
	FLinearColor FlashColor = FLinearColor::Transparent;

	/** Flash decay rate (units per second) */
	static constexpr float FLASH_DECAY_RATE = 3.0f;

	/** Damage flash color (DOOM red) */
	static const FLinearColor DAMAGE_FLASH_COLOR;

	/** Pickup flash color (DOOM gold) */
	static const FLinearColor PICKUP_FLASH_COLOR;

private:
	/** Update all HUD elements from the bound player pawn */
	void UpdateFromPlayer();

	/** Update the screen flash overlay */
	void UpdateFlash(float DeltaTime);

	/** Update the HUD message display */
	void UpdateMessage(float DeltaTime);

	/** Get the UImage* for a key card slot */
	UImage* GetKeyCardWidget(EDoomCard Card) const;

	/** Get the UImage* for an arms slot */
	UImage* GetArmsWidget(int32 SlotIndex) const;
};
