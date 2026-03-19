// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.

#include "DoomGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "HAL/PlatformFileManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoomGameInstance, Log, All);

// =============================================================================
// Constructor
// =============================================================================

UDoomGameInstance::UDoomGameInstance()
{
	// Initialize save slots
	SaveSlots.SetNum(DOOM_MAX_SAVE_SLOTS);
}

// =============================================================================
// UGameInstance overrides
// =============================================================================

void UDoomGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogDoomGameInstance, Log, TEXT("UDoomGameInstance::Init"));

	// Check command line for WAD file path: -wadfile=<path>
	FString CmdWadPath;
	if (FParse::Value(FCommandLine::Get(), TEXT("-wadfile="), CmdWadPath))
	{
		WadFilePath = CmdWadPath;
		UE_LOG(LogDoomGameInstance, Log, TEXT("WAD path from command line: %s"), *WadFilePath);
	}

	// Check command line for PWAD files: -file=<path> (can appear multiple times)
	FString CmdPwadPath;
	if (FParse::Value(FCommandLine::Get(), TEXT("-file="), CmdPwadPath))
	{
		PwadFilePaths.Add(CmdPwadPath);
		UE_LOG(LogDoomGameInstance, Log, TEXT("PWAD from command line: %s"), *CmdPwadPath);
	}

	// If no WAD path from command line, try default locations
	if (WadFilePath.IsEmpty())
	{
		// Look for DOOM2.WAD or DOOM.WAD in the project directory
		const FString ProjectDir = FPaths::ProjectDir();
		const TArray<FString> DefaultWadNames = {
			TEXT("DOOM2.WAD"), TEXT("doom2.wad"),
			TEXT("DOOM.WAD"), TEXT("doom.wad"),
			TEXT("PLUTONIA.WAD"), TEXT("TNT.WAD")
		};

		for (const FString& WadName : DefaultWadNames)
		{
			const FString TestPath = FPaths::Combine(ProjectDir, WadName);
			if (FPaths::FileExists(TestPath))
			{
				WadFilePath = TestPath;
				UE_LOG(LogDoomGameInstance, Log, TEXT("Found WAD at default location: %s"), *WadFilePath);
				break;
			}

			// Also check a "WADs" subdirectory
			const FString WadSubdirPath = FPaths::Combine(ProjectDir, TEXT("WADs"), WadName);
			if (FPaths::FileExists(WadSubdirPath))
			{
				WadFilePath = WadSubdirPath;
				UE_LOG(LogDoomGameInstance, Log, TEXT("Found WAD in WADs directory: %s"), *WadFilePath);
				break;
			}
		}
	}

	// Load user settings from config
	LoadUserSettings();

	// Refresh save slot info from disk
	RefreshSaveSlots();

	if (WadFilePath.IsEmpty())
	{
		UE_LOG(LogDoomGameInstance, Warning,
			TEXT("No WAD file found. Place DOOM.WAD or DOOM2.WAD in the project directory, or use -wadfile= command line argument."));
	}
}

void UDoomGameInstance::Shutdown()
{
	// Save settings and progression on shutdown
	SaveUserSettings();
	SaveProgression();

	Super::Shutdown();
}

// =============================================================================
// Save / Load management
// =============================================================================

FDoomSaveSlotInfo UDoomGameInstance::GetSaveSlotInfo(int32 SlotIndex) const
{
	if (SaveSlots.IsValidIndex(SlotIndex))
	{
		return SaveSlots[SlotIndex];
	}
	return FDoomSaveSlotInfo();
}

bool UDoomGameInstance::SaveGame(int32 SlotIndex, const FString& Description)
{
	if (!SaveSlots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogDoomGameInstance, Error, TEXT("Invalid save slot index: %d"), SlotIndex);
		return false;
	}

	const FString SavePath = GetSaveFilePath(SlotIndex);

	UE_LOG(LogDoomGameInstance, Log, TEXT("Saving game to slot %d: %s"), SlotIndex, *SavePath);

	// Build save data
	// For now, store minimal metadata. Full game state serialization would include:
	// - Player inventory (weapons, ammo, health, armor, keys)
	// - All thinker states (doors, platforms, etc.)
	// - Monster positions and states
	// This matches the original DOOM save format from p_saveg.c

	FDoomSaveSlotInfo& Slot = SaveSlots[SlotIndex];
	Slot.bIsOccupied = true;
	Slot.Description = Description;
	Slot.SaveFilePath = SavePath;

	// TODO: Serialize full game state to the save file
	// For now, write a placeholder
	const FString SaveData = FString::Printf(
		TEXT("{\"description\":\"%s\",\"episode\":%d,\"map\":%d,\"skill\":%d}"),
		*Description, Slot.Episode, Slot.Map, static_cast<int32>(Slot.Skill));

	if (FFileHelper::SaveStringToFile(SaveData, *SavePath))
	{
		UE_LOG(LogDoomGameInstance, Log, TEXT("Save successful: slot %d"), SlotIndex);
		return true;
	}

	UE_LOG(LogDoomGameInstance, Error, TEXT("Failed to write save file: %s"), *SavePath);
	return false;
}

bool UDoomGameInstance::LoadGame(int32 SlotIndex)
{
	if (!SaveSlots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogDoomGameInstance, Error, TEXT("Invalid save slot index: %d"), SlotIndex);
		return false;
	}

	const FDoomSaveSlotInfo& Slot = SaveSlots[SlotIndex];
	if (!Slot.bIsOccupied)
	{
		UE_LOG(LogDoomGameInstance, Warning, TEXT("Save slot %d is empty"), SlotIndex);
		return false;
	}

	const FString SavePath = GetSaveFilePath(SlotIndex);
	UE_LOG(LogDoomGameInstance, Log, TEXT("Loading game from slot %d: %s"), SlotIndex, *SavePath);

	FString SaveData;
	if (!FFileHelper::LoadFileToString(SaveData, *SavePath))
	{
		UE_LOG(LogDoomGameInstance, Error, TEXT("Failed to read save file: %s"), *SavePath);
		return false;
	}

	// TODO: Deserialize full game state from save data
	// This would restore:
	// - Episode, map, skill
	// - Player state (health, armor, weapons, ammo, keys)
	// - All thinker states
	// - Monster positions and states

	UE_LOG(LogDoomGameInstance, Log, TEXT("Load successful: slot %d - %s"), SlotIndex, *Slot.Description);
	return true;
}

bool UDoomGameInstance::DeleteSaveSlot(int32 SlotIndex)
{
	if (!SaveSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FString SavePath = GetSaveFilePath(SlotIndex);

	// Delete the file from disk
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*SavePath))
	{
		PlatformFile.DeleteFile(*SavePath);
	}

	// Clear the slot info
	SaveSlots[SlotIndex] = FDoomSaveSlotInfo();

	UE_LOG(LogDoomGameInstance, Log, TEXT("Deleted save slot %d"), SlotIndex);
	return true;
}

void UDoomGameInstance::RefreshSaveSlots()
{
	for (int32 i = 0; i < DOOM_MAX_SAVE_SLOTS; ++i)
	{
		const FString SavePath = GetSaveFilePath(i);

		if (FPaths::FileExists(SavePath))
		{
			FString SaveData;
			if (FFileHelper::LoadFileToString(SaveData, *SavePath))
			{
				SaveSlots[i].bIsOccupied = true;
				SaveSlots[i].SaveFilePath = SavePath;

				// Parse minimal metadata from save file
				// Full implementation would parse the save header
				if (SaveSlots[i].Description.IsEmpty())
				{
					SaveSlots[i].Description = FString::Printf(TEXT("Save Slot %d"), i + 1);
				}
			}
		}
		else
		{
			SaveSlots[i] = FDoomSaveSlotInfo();
		}
	}
}

// =============================================================================
// User settings
// =============================================================================

void UDoomGameInstance::ApplyUserSettings(const FDoomUserSettings& InSettings)
{
	UserSettings = InSettings;

	UE_LOG(LogDoomGameInstance, Log, TEXT("Applied user settings: SFX=%.2f Music=%.2f Mouse=%.2f AlwaysRun=%d"),
		UserSettings.SfxVolume, UserSettings.MusicVolume, UserSettings.MouseSensitivity,
		UserSettings.bAlwaysRun ? 1 : 0);

	// TODO: Propagate settings to active subsystems
	// - Audio system: update SFX and music volume
	// - Player controller: update mouse sensitivity, always run
	// - Renderer: update gamma, screen size
}

void UDoomGameInstance::SaveUserSettings()
{
	const FString SettingsPath = GetSettingsFilePath();

	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(UserSettings, JsonString))
	{
		FFileHelper::SaveStringToFile(JsonString, *SettingsPath);
		UE_LOG(LogDoomGameInstance, Log, TEXT("User settings saved to %s"), *SettingsPath);
	}
}

void UDoomGameInstance::LoadUserSettings()
{
	const FString SettingsPath = GetSettingsFilePath();

	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *SettingsPath))
	{
		if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &UserSettings))
		{
			UE_LOG(LogDoomGameInstance, Log, TEXT("User settings loaded from %s"), *SettingsPath);
			return;
		}
	}

	// If no settings file exists, use defaults
	UE_LOG(LogDoomGameInstance, Log, TEXT("No user settings found, using defaults"));
	UserSettings = FDoomUserSettings();
}

// =============================================================================
// Player progression
// =============================================================================

void UDoomGameInstance::UpdateProgression(int32 Kills, int32 Items, int32 Secrets, int32 LevelTics)
{
	Progression.TotalKills += Kills;
	Progression.TotalItems += Items;
	Progression.TotalSecrets += Secrets;
	Progression.TotalTicsPlayed += LevelTics;
}

void UDoomGameInstance::RecordDeath()
{
	Progression.TotalDeaths++;
}

void UDoomGameInstance::SaveProgression()
{
	const FString ProgressionPath = GetProgressionFilePath();

	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(Progression, JsonString))
	{
		FFileHelper::SaveStringToFile(JsonString, *ProgressionPath);
		UE_LOG(LogDoomGameInstance, Log, TEXT("Progression saved"));
	}
}

// =============================================================================
// Private helpers
// =============================================================================

FString UDoomGameInstance::GetSaveFilePath(int32 SlotIndex) const
{
	// Save files in the project's Saved directory, matching DOOM's doomsav0.dsg format
	return FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SaveGames"),
		FString::Printf(TEXT("doomsav%d.dsg"), SlotIndex));
}

FString UDoomGameInstance::GetSettingsFilePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"), TEXT("DoomSettings.json"));
}

FString UDoomGameInstance::GetProgressionFilePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"), TEXT("DoomProgression.json"));
}
