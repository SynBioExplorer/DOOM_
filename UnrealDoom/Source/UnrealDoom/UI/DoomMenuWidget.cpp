// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomMenuWidget implementation - DOOM main menu system.
// Mirrors m_menu.c: M_Drawer, M_Ticker, M_Responder, etc.

#include "DoomMenuWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UDoomMenuWidget::UDoomMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Allow this widget to receive keyboard focus
	SetIsFocusable(true);
}

void UDoomMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeMenuItems();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDoomMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsOpen)
	{
		return;
	}

	// Animate skull cursor (toggle between two frames like original DOOM)
	SkullAnimTimer += InDeltaTime;
	if (SkullAnimTimer >= SKULL_ANIM_RATE)
	{
		SkullAnimTimer -= SKULL_ANIM_RATE;
		SkullAnimFrame = (SkullAnimFrame + 1) % 2;

		// In a full implementation, swap skull cursor texture between
		// M_SKULL1 and M_SKULL2 based on SkullAnimFrame
	}
}

FReply UDoomMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bIsOpen)
	{
		return FReply::Unhandled();
	}

	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Up || Key == EKeys::W)
	{
		MenuUp();
		return FReply::Handled();
	}
	if (Key == EKeys::Down || Key == EKeys::S)
	{
		MenuDown();
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		MenuSelect();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::BackSpace)
	{
		MenuBack();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// =============================================================================
// Menu control
// =============================================================================

void UDoomMenuWidget::OpenMenu()
{
	bIsOpen = true;
	SetVisibility(ESlateVisibility::Visible);

	MenuStack.Empty();
	IndexStack.Empty();
	CurrentPage = EDoomMenuPage::MainMenu;
	CurrentMenuIndex = 0;

	SetItemsForPage(CurrentPage);
	RebuildMenuDisplay();
	UpdateCursorPosition();

	// Take keyboard focus
	SetFocus();
}

void UDoomMenuWidget::CloseMenu()
{
	bIsOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);

	MenuStack.Empty();
	IndexStack.Empty();
}

void UDoomMenuWidget::NavigateToPage(EDoomMenuPage Page)
{
	// Push current state onto stack
	MenuStack.Push(CurrentPage);
	IndexStack.Push(CurrentMenuIndex);

	CurrentPage = Page;
	CurrentMenuIndex = 0;

	SetItemsForPage(CurrentPage);
	RebuildMenuDisplay();
	UpdateCursorPosition();
}

void UDoomMenuWidget::GoBack()
{
	if (MenuStack.Num() > 0)
	{
		CurrentPage = MenuStack.Pop();
		CurrentMenuIndex = IndexStack.Pop();

		SetItemsForPage(CurrentPage);
		RebuildMenuDisplay();
		UpdateCursorPosition();

		PlayMenuSound(SFX_MENU_BACK);
	}
	else
	{
		// At root menu, close
		CloseMenu();
	}
}

// =============================================================================
// Input handling
// =============================================================================

void UDoomMenuWidget::MenuUp()
{
	if (CurrentItems.Num() == 0)
	{
		return;
	}

	// Move up, wrapping around. Skip non-selectable items.
	int32 NewIndex = CurrentMenuIndex;
	do
	{
		NewIndex--;
		if (NewIndex < 0)
		{
			NewIndex = CurrentItems.Num() - 1;
		}
	}
	while (!CurrentItems[NewIndex].bSelectable && NewIndex != CurrentMenuIndex);

	CurrentMenuIndex = NewIndex;
	UpdateCursorPosition();
	PlayMenuSound(SFX_MENU_MOVE);
}

void UDoomMenuWidget::MenuDown()
{
	if (CurrentItems.Num() == 0)
	{
		return;
	}

	int32 NewIndex = CurrentMenuIndex;
	do
	{
		NewIndex++;
		if (NewIndex >= CurrentItems.Num())
		{
			NewIndex = 0;
		}
	}
	while (!CurrentItems[NewIndex].bSelectable && NewIndex != CurrentMenuIndex);

	CurrentMenuIndex = NewIndex;
	UpdateCursorPosition();
	PlayMenuSound(SFX_MENU_MOVE);
}

void UDoomMenuWidget::MenuSelect()
{
	if (CurrentItems.Num() == 0 || !CurrentItems.IsValidIndex(CurrentMenuIndex))
	{
		return;
	}

	if (!CurrentItems[CurrentMenuIndex].bSelectable)
	{
		return;
	}

	PlayMenuSound(SFX_MENU_SELECT);
	HandleSelection(CurrentMenuIndex);
}

void UDoomMenuWidget::MenuBack()
{
	GoBack();
}

// =============================================================================
// Configuration
// =============================================================================

void UDoomMenuWidget::SetGameMode(EDoomGameMode InGameMode)
{
	GameMode = InGameMode;
	InitializeMenuItems();
}

// =============================================================================
// Private implementation
// =============================================================================

void UDoomMenuWidget::InitializeMenuItems()
{
	// -------------------------------------------------------------------------
	// Main menu: New Game, Options, Load Game, Save Game, Quit Game
	// -------------------------------------------------------------------------
	MainMenuItems.Empty();
	MainMenuItems.Add({ TEXT("New Game"), true, 0 });
	MainMenuItems.Add({ TEXT("Options"), true, 1 });
	MainMenuItems.Add({ TEXT("Load Game"), true, 2 });
	MainMenuItems.Add({ TEXT("Save Game"), true, 3 });
	MainMenuItems.Add({ TEXT("Quit Game"), true, 4 });

	// -------------------------------------------------------------------------
	// Episode selection (DOOM 1 / Ultimate DOOM)
	// -------------------------------------------------------------------------
	EpisodeItems.Empty();
	EpisodeItems.Add({ TEXT("Knee-Deep in the Dead"), true, 1 });
	EpisodeItems.Add({ TEXT("The Shores of Hell"), true, 2 });
	EpisodeItems.Add({ TEXT("Inferno"), true, 3 });

	// Thy Flesh Consumed only available in retail (Ultimate DOOM)
	if (GameMode == EDoomGameMode::Retail)
	{
		EpisodeItems.Add({ TEXT("Thy Flesh Consumed"), true, 4 });
	}

	// In shareware mode, only episode 1 is available
	if (GameMode == EDoomGameMode::Shareware)
	{
		for (int32 i = 1; i < EpisodeItems.Num(); ++i)
		{
			EpisodeItems[i].bSelectable = false;
		}
	}

	// -------------------------------------------------------------------------
	// Skill selection
	// -------------------------------------------------------------------------
	SkillItems.Empty();
	SkillItems.Add({ TEXT("I'm Too Young To Die"), true, static_cast<int32>(EDoomSkill::Baby) });
	SkillItems.Add({ TEXT("Hey, Not Too Rough"), true, static_cast<int32>(EDoomSkill::Easy) });
	SkillItems.Add({ TEXT("Hurt Me Plenty"), true, static_cast<int32>(EDoomSkill::Medium) });
	SkillItems.Add({ TEXT("Ultra-Violence"), true, static_cast<int32>(EDoomSkill::Hard) });
	SkillItems.Add({ TEXT("Nightmare!"), true, static_cast<int32>(EDoomSkill::Nightmare) });
}

void UDoomMenuWidget::SetItemsForPage(EDoomMenuPage Page)
{
	switch (Page)
	{
	case EDoomMenuPage::MainMenu:
		CurrentItems = MainMenuItems;
		break;
	case EDoomMenuPage::EpisodeSelect:
		CurrentItems = EpisodeItems;
		break;
	case EDoomMenuPage::SkillSelect:
		CurrentItems = SkillItems;
		break;
	case EDoomMenuPage::OptionsMenu:
	case EDoomMenuPage::LoadGame:
	case EDoomMenuPage::SaveGame:
		// These pages would have their own items in a full implementation
		CurrentItems.Empty();
		break;
	}
}

void UDoomMenuWidget::RebuildMenuDisplay()
{
	if (!MenuItemContainer)
	{
		return;
	}

	// Clear existing children
	MenuItemContainer->ClearChildren();

	// Create a text block for each menu item
	for (int32 i = 0; i < CurrentItems.Num(); ++i)
	{
		const FDoomMenuItem& Item = CurrentItems[i];

		UTextBlock* ItemText = NewObject<UTextBlock>(this);
		if (ItemText)
		{
			ItemText->SetText(FText::FromString(Item.DisplayText));

			// Style: selectable items are white, non-selectable are grey
			if (Item.bSelectable)
			{
				ItemText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			}
			else
			{
				ItemText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f)));
			}

			UVerticalBoxSlot* Slot = MenuItemContainer->AddChildToVerticalBox(ItemText);
			if (Slot)
			{
				Slot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));
			}
		}
	}
}

void UDoomMenuWidget::UpdateCursorPosition()
{
	if (!SkullCursor || !MenuItemContainer)
	{
		return;
	}

	// Position the skull cursor next to the current menu item.
	// In a full UMG implementation, this would adjust the cursor's
	// vertical position to align with the selected item in the VerticalBox.
	// The exact positioning depends on the Widget Blueprint layout.

	// For now, we set the render transform Y offset based on item index and spacing.
	const float ItemHeight = 28.0f; // Approximate height per menu item
	const float YOffset = CurrentMenuIndex * ItemHeight;
	SkullCursor->SetRenderTranslation(FVector2D(0.0f, YOffset));
}

void UDoomMenuWidget::PlayMenuSound(int32 SoundId)
{
	// In a full implementation, this would call into the DOOM audio system:
	// UDoomAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UDoomAudioSubsystem>();
	// Audio->PlaySound(SoundId);
	//
	// Sound IDs:
	// SFX_MENU_MOVE = sfx_pistol (pistol fire sound for menu cursor movement)
	// SFX_MENU_SELECT = sfx_swtchn (switch activation for menu selection)
	// SFX_MENU_BACK = sfx_swtchx (switch deactivation for going back)
}

void UDoomMenuWidget::HandleSelection(int32 ItemIndex)
{
	if (!CurrentItems.IsValidIndex(ItemIndex))
	{
		return;
	}

	const FDoomMenuItem& Item = CurrentItems[ItemIndex];

	// Broadcast the delegate
	OnMenuItemSelected.Broadcast(CurrentPage, ItemIndex, Item.Value);

	// Handle page transitions based on current page
	switch (CurrentPage)
	{
	case EDoomMenuPage::MainMenu:
		switch (ItemIndex)
		{
		case 0: // New Game
			if (GameMode == EDoomGameMode::Commercial)
			{
				// DOOM 2 skips episode select, goes straight to skill
				NavigateToPage(EDoomMenuPage::SkillSelect);
			}
			else
			{
				// DOOM 1 shows episode select first
				NavigateToPage(EDoomMenuPage::EpisodeSelect);
			}
			break;
		case 1: // Options
			NavigateToPage(EDoomMenuPage::OptionsMenu);
			break;
		case 2: // Load Game
			NavigateToPage(EDoomMenuPage::LoadGame);
			break;
		case 3: // Save Game
			NavigateToPage(EDoomMenuPage::SaveGame);
			break;
		case 4: // Quit Game
			// Quit is handled by the delegate receiver
			break;
		}
		break;

	case EDoomMenuPage::EpisodeSelect:
		// After selecting an episode, go to skill select
		NavigateToPage(EDoomMenuPage::SkillSelect);
		break;

	case EDoomMenuPage::SkillSelect:
		// After selecting skill, the game starts.
		// The delegate receiver (game mode) handles the actual game start.
		// Close the menu.
		CloseMenu();
		break;

	default:
		break;
	}
}
