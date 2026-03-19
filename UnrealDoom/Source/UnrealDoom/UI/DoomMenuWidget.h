#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomMenuWidget - Recreation of the DOOM main menu system in UMG.
// Handles main menu, skill selection, episode selection, and navigation.
// Equivalent to m_menu.c.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/DoomTypes.h"
#include "DoomMenuWidget.generated.h"

class UTextBlock;
class UImage;
class UVerticalBox;

/**
 * Menu page identifiers matching the original DOOM menu system.
 */
UENUM(BlueprintType)
enum class EDoomMenuPage : uint8
{
	MainMenu = 0,      // M_DOOM - New Game, Options, Load, Save, Quit
	EpisodeSelect = 1, // M_EPISOD - Knee-Deep, Shores, Inferno, Thy Flesh
	SkillSelect = 2,   // M_NEWG / M_SKILL
	OptionsMenu = 3,   // M_OPTTTL
	LoadGame = 4,      // M_LOADG
	SaveGame = 5       // M_SAVEG
};

/**
 * A single menu item entry.
 */
USTRUCT(BlueprintType)
struct FDoomMenuItem
{
	GENERATED_BODY()

	/** Display text for this menu item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Menu")
	FString DisplayText;

	/** Whether this item is selectable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Menu")
	bool bSelectable = true;

	/** Identifier value (skill level, episode number, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Menu")
	int32 Value = 0;
};

/**
 * Delegate fired when a menu item is selected.
 * @param MenuPage - The page the selection was made on
 * @param ItemIndex - Index of the selected item
 * @param ItemValue - The value associated with the selected item
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnMenuItemSelected,
	EDoomMenuPage, MenuPage,
	int32, ItemIndex,
	int32, ItemValue
);

/**
 * UDoomMenuWidget
 *
 * Recreates the DOOM main menu as a UMG widget. Supports keyboard/gamepad
 * navigation with the skull cursor, sound effects on move/select, and
 * a stack-based submenu system matching the original menu behavior.
 *
 * Menu flow:
 *   Main Menu -> New Game -> Episode Select (DOOM 1) -> Skill Select -> Start
 *   Main Menu -> New Game -> Skill Select (DOOM 2) -> Start
 *   Main Menu -> Load Game -> Slot Select
 *   Main Menu -> Save Game -> Slot Select
 *   Main Menu -> Options -> ...
 *   Main Menu -> Quit Game -> Confirm
 */
UCLASS()
class UNREALDOOM_API UDoomMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDoomMenuWidget(const FObjectInitializer& ObjectInitializer);

	// =========================================================================
	// Menu control
	// =========================================================================

	/** Open the menu starting at the main page */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void OpenMenu();

	/** Close the menu entirely */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void CloseMenu();

	/** Navigate to a specific menu page */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void NavigateToPage(EDoomMenuPage Page);

	/** Go back to the previous menu page (or close if at root) */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void GoBack();

	// =========================================================================
	// Input handling
	// =========================================================================

	/** Move cursor up */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void MenuUp();

	/** Move cursor down */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void MenuDown();

	/** Select the current item */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void MenuSelect();

	/** Go back (Escape key) */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void MenuBack();

	// =========================================================================
	// State queries
	// =========================================================================

	/** @return Whether the menu is currently visible */
	UFUNCTION(BlueprintPure, Category = "Doom|Menu")
	bool IsMenuOpen() const { return bIsOpen; }

	/** @return The current menu page */
	UFUNCTION(BlueprintPure, Category = "Doom|Menu")
	EDoomMenuPage GetCurrentPage() const { return CurrentPage; }

	/** @return The currently highlighted item index */
	UFUNCTION(BlueprintPure, Category = "Doom|Menu")
	int32 GetCurrentIndex() const { return CurrentMenuIndex; }

	// =========================================================================
	// Configuration
	// =========================================================================

	/** Set the game mode (affects whether episode select is shown) */
	UFUNCTION(BlueprintCallable, Category = "Doom|Menu")
	void SetGameMode(EDoomGameMode InGameMode);

	// =========================================================================
	// Delegates
	// =========================================================================

	/** Fired when a menu item is selected (confirmed with Enter) */
	UPROPERTY(BlueprintAssignable, Category = "Doom|Menu")
	FOnMenuItemSelected OnMenuItemSelected;

protected:
	// =========================================================================
	// UUserWidget overrides
	// =========================================================================

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// =========================================================================
	// Bound widgets
	// =========================================================================

	/** Container for menu item text entries */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> MenuItemContainer;

	/** The skull cursor image (animated, bounces left/right) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkullCursor;

	/** Title image for the current menu page */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> MenuTitle;

	// =========================================================================
	// Menu data
	// =========================================================================

	/** Currently displayed menu page */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Menu")
	EDoomMenuPage CurrentPage = EDoomMenuPage::MainMenu;

	/** Currently highlighted menu item index */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Menu")
	int32 CurrentMenuIndex = 0;

	/** Stack of previous menu pages for GoBack() navigation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Menu")
	TArray<EDoomMenuPage> MenuStack;

	/** Stack of cursor positions for each menu page */
	TArray<int32> IndexStack;

	/** Whether the menu is open */
	bool bIsOpen = false;

	/** The current game mode (determines episode select availability) */
	EDoomGameMode GameMode = EDoomGameMode::Retail;

	// =========================================================================
	// Menu item definitions
	// =========================================================================

	/** Main menu items */
	TArray<FDoomMenuItem> MainMenuItems;

	/** Episode selection items (DOOM 1 only) */
	TArray<FDoomMenuItem> EpisodeItems;

	/** Skill selection items */
	TArray<FDoomMenuItem> SkillItems;

	/** Currently active menu items */
	TArray<FDoomMenuItem> CurrentItems;

	// =========================================================================
	// Skull cursor animation
	// =========================================================================

	/** Skull cursor animation timer */
	float SkullAnimTimer = 0.0f;

	/** Skull animation frame (0 or 1, toggling for DOOM skull bounce) */
	int32 SkullAnimFrame = 0;

	/** Skull animation rate in seconds */
	static constexpr float SKULL_ANIM_RATE = 0.5f;

	// =========================================================================
	// Sound effect names (to be resolved by audio system)
	// =========================================================================

	/** Sound played when navigating menu items (sfx_pstol - pistol sound) */
	static constexpr int32 SFX_MENU_MOVE = 1; // sfx_pistol

	/** Sound played when selecting/activating a menu item (sfx_swtchn) */
	static constexpr int32 SFX_MENU_SELECT = 2; // sfx_swtchn

	/** Sound played when backing out of a menu (sfx_swtchx) */
	static constexpr int32 SFX_MENU_BACK = 3; // sfx_swtchx

private:
	/** Initialize all menu item arrays */
	void InitializeMenuItems();

	/** Rebuild the displayed menu items for the current page */
	void RebuildMenuDisplay();

	/** Update the skull cursor position to match CurrentMenuIndex */
	void UpdateCursorPosition();

	/** Play a menu sound effect */
	void PlayMenuSound(int32 SoundId);

	/** Handle a confirmed selection on the current page */
	void HandleSelection(int32 ItemIndex);

	/** Set up the items array for a given page */
	void SetItemsForPage(EDoomMenuPage Page);
};
