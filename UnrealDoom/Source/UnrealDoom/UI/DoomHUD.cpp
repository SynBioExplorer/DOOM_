// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomHUDWidget implementation - DOOM status bar recreation in UMG.
// Mirrors st_stuff.c: ST_Ticker, ST_Drawer, ST_updateFaceWidget, etc.

#include "DoomHUD.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "DoomPlayerPawn.h"

// Static flash colors
const FLinearColor UDoomHUDWidget::DAMAGE_FLASH_COLOR = FLinearColor(1.0f, 0.0f, 0.0f, 0.5f);
const FLinearColor UDoomHUDWidget::PICKUP_FLASH_COLOR = FLinearColor(1.0f, 0.85f, 0.0f, 0.5f);

UDoomHUDWidget::UDoomHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDoomHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize message text as hidden
	if (MessageText)
	{
		MessageText->SetVisibility(ESlateVisibility::Hidden);
	}

	// Initialize flash overlay as transparent
	if (ScreenFlashOverlay)
	{
		ScreenFlashOverlay->SetColorAndOpacity(FLinearColor::Transparent);
		ScreenFlashOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// Initialize all key card indicators as hidden
	for (int32 i = 0; i < 6; ++i)
	{
		UImage* KeyWidget = GetKeyCardWidget(static_cast<EDoomCard>(i));
		if (KeyWidget)
		{
			KeyWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// Set default stat displays
	SetHealth(100);
	SetArmor(0);
	SetAmmo(50);
}

void UDoomHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Update HUD from player state if bound
	if (BoundPlayerPawn)
	{
		UpdateFromPlayer();
	}

	// Update flash overlay
	UpdateFlash(InDeltaTime);

	// Update message timer
	UpdateMessage(InDeltaTime);
}

// =============================================================================
// Player binding
// =============================================================================

void UDoomHUDWidget::BindToPlayer(ADoomPlayerPawn* InPlayerPawn)
{
	BoundPlayerPawn = InPlayerPawn;

	// Force a full refresh
	CachedHealth = -1;
	CachedArmor = -1;
	CachedAmmo = -1;
}

// =============================================================================
// Stat setters
// =============================================================================

void UDoomHUDWidget::SetHealth(int32 Value)
{
	if (CachedHealth == Value)
	{
		return;
	}
	CachedHealth = Value;

	if (HealthText)
	{
		HealthText->SetText(FText::AsNumber(Value));
	}
}

void UDoomHUDWidget::SetArmor(int32 Value)
{
	if (CachedArmor == Value)
	{
		return;
	}
	CachedArmor = Value;

	if (ArmorText)
	{
		ArmorText->SetText(FText::AsNumber(Value));
	}
}

void UDoomHUDWidget::SetAmmo(int32 Value)
{
	if (CachedAmmo == Value)
	{
		return;
	}
	CachedAmmo = Value;

	if (CurrentAmmoText)
	{
		CurrentAmmoText->SetText(FText::AsNumber(Value));
	}
	if (AmmoText)
	{
		AmmoText->SetText(FText::AsNumber(Value));
	}
}

void UDoomHUDWidget::SetKeyCard(EDoomCard Card, bool bHasKey)
{
	const int32 Index = static_cast<int32>(Card);
	if (Index < 0 || Index >= 6)
	{
		return;
	}

	bKeyCards[Index] = bHasKey;

	UImage* KeyWidget = GetKeyCardWidget(Card);
	if (KeyWidget)
	{
		KeyWidget->SetVisibility(bHasKey ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UDoomHUDWidget::SetWeaponAvailable(int32 SlotIndex, bool bAvailable)
{
	if (SlotIndex < 0 || SlotIndex >= 7)
	{
		return;
	}

	bWeaponSlots[SlotIndex] = bAvailable;

	UImage* ArmsWidget = GetArmsWidget(SlotIndex);
	if (ArmsWidget)
	{
		// Show available weapons at full opacity, unavailable at reduced opacity
		ArmsWidget->SetColorAndOpacity(bAvailable
			? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
			: FLinearColor(0.3f, 0.3f, 0.3f, 0.5f));
	}
}

void UDoomHUDWidget::SetFaceIndex(int32 FaceIndex)
{
	// The face widget texture would be set based on FaceIndex.
	// In a full implementation this would index into the DOOM face sprite atlas:
	// STFST00-STFST40 (5 health levels x multiple directions + special states)
	// For now, just store the index. The Blueprint or a face manager handles the actual texture.
	if (FaceWidget)
	{
		// Face texture assignment would go here once the sprite atlas is loaded.
		// FaceWidget->SetBrushFromTexture(FaceTextures[FaceIndex]);
	}
}

// =============================================================================
// HUD message
// =============================================================================

void UDoomHUDWidget::ShowMessage(const FString& Message)
{
	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Message));
		MessageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	MessageTimer = MessageDisplayTime;
}

// =============================================================================
// Screen flash
// =============================================================================

void UDoomHUDWidget::TriggerDamageFlash(float Intensity)
{
	FlashIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	FlashColor = DAMAGE_FLASH_COLOR;
}

void UDoomHUDWidget::TriggerPickupFlash(float Intensity)
{
	FlashIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	FlashColor = PICKUP_FLASH_COLOR;
}

// =============================================================================
// Private update methods
// =============================================================================

void UDoomHUDWidget::UpdateFromPlayer()
{
	if (!BoundPlayerPawn)
	{
		return;
	}

	// In a full implementation, this would read from a DoomPlayerComponent:
	// SetHealth(PlayerComp->Health);
	// SetArmor(PlayerComp->ArmorPoints);
	// SetAmmo(PlayerComp->GetCurrentAmmoCount());
	// for (int32 i = 0; i < 6; ++i)
	//     SetKeyCard(static_cast<EDoomCard>(i), PlayerComp->Cards[i]);
	// for (int32 i = 0; i < 7; ++i)
	//     SetWeaponAvailable(i, PlayerComp->WeaponOwned[i]);
	// SetFaceIndex(PlayerComp->GetFaceIndex());
}

void UDoomHUDWidget::UpdateFlash(float DeltaTime)
{
	if (FlashIntensity > 0.0f)
	{
		FlashIntensity -= FLASH_DECAY_RATE * DeltaTime;

		if (FlashIntensity <= 0.0f)
		{
			FlashIntensity = 0.0f;
			if (ScreenFlashOverlay)
			{
				ScreenFlashOverlay->SetColorAndOpacity(FLinearColor::Transparent);
			}
		}
		else if (ScreenFlashOverlay)
		{
			FLinearColor OverlayColor = FlashColor;
			OverlayColor.A = FlashIntensity * FlashColor.A;
			ScreenFlashOverlay->SetColorAndOpacity(OverlayColor);
		}
	}
}

void UDoomHUDWidget::UpdateMessage(float DeltaTime)
{
	if (MessageTimer > 0.0f)
	{
		MessageTimer -= DeltaTime;

		if (MessageTimer <= 0.0f)
		{
			MessageTimer = 0.0f;
			if (MessageText)
			{
				MessageText->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

UImage* UDoomHUDWidget::GetKeyCardWidget(EDoomCard Card) const
{
	switch (Card)
	{
	case EDoomCard::BlueCard:    return KeyCard_Blue;
	case EDoomCard::YellowCard:  return KeyCard_Yellow;
	case EDoomCard::RedCard:     return KeyCard_Red;
	case EDoomCard::BlueSkull:   return KeySkull_Blue;
	case EDoomCard::YellowSkull: return KeySkull_Yellow;
	case EDoomCard::RedSkull:    return KeySkull_Red;
	default:                     return nullptr;
	}
}

UImage* UDoomHUDWidget::GetArmsWidget(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return Arms_Slot1;
	case 1: return Arms_Slot2;
	case 2: return Arms_Slot3;
	case 3: return Arms_Slot4;
	case 4: return Arms_Slot5;
	case 5: return Arms_Slot6;
	case 6: return Arms_Slot7;
	default: return nullptr;
	}
}
