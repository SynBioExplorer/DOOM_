#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomGameInstance - Persistent game state that survives level transitions.
// Handles WAD file path configuration, save/load slot management,
// player progression, and user settings.

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/DoomTypes.h"
#include "DoomGameInstance.generated.h"

/** Maximum number of save game slots (matches original DOOM) */
static constexpr int32 DOOM_MAX_SAVE_SLOTS = 6;

/**
 * FDoomSaveSlotInfo - Metadata for a single save game slot.
 */
USTRUCT(BlueprintType)
struct FDoomSaveSlotInfo
{
	GENERATED_BODY()

	/** Whether this slot contains a valid save */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Save")
	bool bIsOccupied = false;

	/** Display name / description of the save */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Save")
	FString Description;

	/** Episode at time of save */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Save")
	int32 Episode = 0;

	/** Map at time of save */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Save")
	int32 Map = 0;

	/** Skill level at time of save */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Save")
	EDoomSkill Skill = EDoomSkill::Medium;

	/** File path to the save data on disk */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Save")
	FString SaveFilePath;
};

/**
 * FDoomUserSettings - User-configurable settings persisted across sessions.
 */
USTRUCT(BlueprintType)
struct FDoomUserSettings
{
	GENERATED_BODY()

	/** Sound effects volume (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings")
	float SfxVolume = 0.8f;

	/** Music volume (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings")
	float MusicVolume = 0.8f;

	/** Mouse sensitivity for horizontal look */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings")
	float MouseSensitivity = 5.0f;

	/** Whether to always run (toggle run behavior) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings")
	bool bAlwaysRun = false;

	/** Screen size / render resolution scale (1-11, matching original DOOM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings", meta = (ClampMin = "1", ClampMax = "11"))
	int32 ScreenSize = 10;

	/** Show messages on screen (pickup messages, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings")
	bool bShowMessages = true;

	/** Gamma correction level (0-4, matching original DOOM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings", meta = (ClampMin = "0", ClampMax = "4"))
	int32 GammaLevel = 0;
};

/**
 * FDoomPlayerProgression - Tracks cumulative player stats across sessions.
 */
USTRUCT(BlueprintType)
struct FDoomPlayerProgression
{
	GENERATED_BODY()

	/** Total monsters killed across all sessions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	int32 TotalKills = 0;

	/** Total items collected across all sessions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	int32 TotalItems = 0;

	/** Total secrets found across all sessions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	int32 TotalSecrets = 0;

	/** Total deaths */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	int32 TotalDeaths = 0;

	/** Total tics played (for total playtime calculation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	int64 TotalTicsPlayed = 0;

	/** Highest episode reached */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	int32 HighestEpisode = 0;

	/** Highest map reached */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	int32 HighestMap = 0;
};

/**
 * UDoomGameInstance - Persistent game state across level loads.
 *
 * This UGameInstance subclass provides:
 * - WAD file path configuration (from config file or command line)
 * - Save/load game slot management (6 slots, matching original)
 * - Player progression tracking (cumulative stats)
 * - User settings persistence (volume, sensitivity, etc.)
 *
 * The GameInstance persists for the entire application lifetime,
 * surviving map transitions, unlike the GameMode which is per-map.
 */
UCLASS(Blueprintable, ClassGroup = "Doom")
class UNREALDOOM_API UDoomGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UDoomGameInstance();

	// =========================================================================
	// UGameInstance overrides
	// =========================================================================

	/** Initialize from command line args and config files. */
	virtual void Init() override;

	/** Clean up on shutdown. */
	virtual void Shutdown() override;

	// =========================================================================
	// WAD file configuration
	// =========================================================================

	/** @return The configured WAD file path */
	UFUNCTION(BlueprintPure, Category = "Doom|Config")
	FString GetWadFilePath() const { return WadFilePath; }

	/**
	 * Set the WAD file path. Does not reload - call InitializeDoom on the GameMode.
	 * @param InPath - Absolute path to the WAD file
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Config")
	void SetWadFilePath(const FString& InPath) { WadFilePath = InPath; }

	/** @return The configured PWAD file paths (mod WADs) */
	UFUNCTION(BlueprintPure, Category = "Doom|Config")
	TArray<FString> GetPwadFilePaths() const { return PwadFilePaths; }

	/**
	 * Add a PWAD (mod) file path to load after the IWAD.
	 * @param InPath - Absolute path to the PWAD file
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Config")
	void AddPwadFilePath(const FString& InPath) { PwadFilePaths.Add(InPath); }

	// =========================================================================
	// Save / Load management
	// =========================================================================

	/**
	 * Get info about a save game slot.
	 * @param SlotIndex - Slot number (0 to DOOM_MAX_SAVE_SLOTS-1)
	 * @return The slot info, or default if invalid index
	 */
	UFUNCTION(BlueprintPure, Category = "Doom|Save")
	FDoomSaveSlotInfo GetSaveSlotInfo(int32 SlotIndex) const;

	/**
	 * Save the current game state to a slot.
	 * @param SlotIndex - Slot number (0 to DOOM_MAX_SAVE_SLOTS-1)
	 * @param Description - User-visible description for this save
	 * @return true if save succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Save")
	bool SaveGame(int32 SlotIndex, const FString& Description);

	/**
	 * Load a saved game from a slot.
	 * @param SlotIndex - Slot number to load
	 * @return true if load succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Save")
	bool LoadGame(int32 SlotIndex);

	/**
	 * Delete a save game slot.
	 * @param SlotIndex - Slot number to delete
	 * @return true if deletion succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Save")
	bool DeleteSaveSlot(int32 SlotIndex);

	/** Refresh the save slot info from disk. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Save")
	void RefreshSaveSlots();

	// =========================================================================
	// User settings
	// =========================================================================

	/** @return Current user settings */
	UFUNCTION(BlueprintPure, Category = "Doom|Settings")
	FDoomUserSettings GetUserSettings() const { return UserSettings; }

	/** @return Reference to user settings for modification */
	FDoomUserSettings& GetUserSettingsRef() { return UserSettings; }

	/**
	 * Apply user settings (updates audio volumes, mouse sensitivity, etc.)
	 * @param InSettings - The settings to apply
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Settings")
	void ApplyUserSettings(const FDoomUserSettings& InSettings);

	/** Save user settings to the config file. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Settings")
	void SaveUserSettings();

	/** Load user settings from the config file. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Settings")
	void LoadUserSettings();

	// =========================================================================
	// Player progression
	// =========================================================================

	/** @return Current player progression stats */
	UFUNCTION(BlueprintPure, Category = "Doom|Progression")
	FDoomPlayerProgression GetProgression() const { return Progression; }

	/**
	 * Update progression after completing a level.
	 * @param Kills - Kills in the completed level
	 * @param Items - Items collected
	 * @param Secrets - Secrets found
	 * @param LevelTics - Tics spent in the level
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Progression")
	void UpdateProgression(int32 Kills, int32 Items, int32 Secrets, int32 LevelTics);

	/** Record a player death. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Progression")
	void RecordDeath();

	/** Save progression data to disk. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Progression")
	void SaveProgression();

protected:
	// =========================================================================
	// Configuration
	// =========================================================================

	/** Path to the IWAD file (DOOM.WAD, DOOM2.WAD, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Config")
	FString WadFilePath;

	/** Paths to PWAD files (mods/patches) to load on top of the IWAD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Config")
	TArray<FString> PwadFilePaths;

	// =========================================================================
	// Save game state
	// =========================================================================

	/** Information about each save game slot */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Save")
	TArray<FDoomSaveSlotInfo> SaveSlots;

	// =========================================================================
	// User settings
	// =========================================================================

	/** User-configurable settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Settings")
	FDoomUserSettings UserSettings;

	// =========================================================================
	// Progression
	// =========================================================================

	/** Cumulative player statistics */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Progression")
	FDoomPlayerProgression Progression;

private:
	/** Build the save file path for a given slot index. */
	FString GetSaveFilePath(int32 SlotIndex) const;

	/** Build the path for the user settings config file. */
	FString GetSettingsFilePath() const;

	/** Build the path for the progression data file. */
	FString GetProgressionFilePath() const;
};
