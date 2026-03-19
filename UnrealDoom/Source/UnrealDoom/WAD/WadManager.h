#pragma once

#include "CoreMinimal.h"
#include "WadTypes.h"
#include "WadFile.h"
#include "WadManager.generated.h"

/**
 * UWadManager - Manages multiple WAD files and provides unified lump access.
 *
 * Port of the global WAD management from linuxdoom-1.10 w_wad.c.
 * Handles loading multiple WAD files (IWAD + PWADs), building a combined
 * lump directory where later files override earlier ones, and detecting
 * the game mode based on loaded content.
 *
 * Usage mirrors the original W_InitMultipleFiles / W_CheckNumForName / etc.
 * with the key design that lump name searches scan backwards so PWAD entries
 * take precedence over IWAD entries.
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UWadManager : public UObject
{
	GENERATED_BODY()

public:
	UWadManager();

	// ========================================================================
	// File Management
	// ========================================================================

	/**
	 * Add a single WAD or lump file to the manager.
	 * Equivalent to W_AddFile from the original source.
	 * @param Path - Path to the WAD file to add
	 * @return true if the file was successfully added
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	bool AddFile(const FString& Path);

	/**
	 * Initialize from multiple files. Equivalent to W_InitMultipleFiles.
	 * Opens all files, loads headers, and builds the combined lump directory.
	 * Lump names can appear multiple times; the name searcher looks backwards
	 * so a later file overrides all earlier ones.
	 * @param FileNames - Array of WAD file paths to load
	 * @return true if at least one file was loaded successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	bool InitMultipleFiles(const TArray<FString>& FileNames);

	// ========================================================================
	// Lump Access (searches across all loaded WADs)
	// ========================================================================

	/**
	 * Find a lump by name across all loaded WADs.
	 * Searches backwards so the last WAD added wins for overrides.
	 * Equivalent to W_CheckNumForName (returns -1 if not found).
	 * @param Name - Lump name (case insensitive, max 8 chars)
	 * @return Global lump index, or -1 if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 FindLump(const FString& Name) const;

	/**
	 * Find a lump by name, asserting it exists. Equivalent to W_GetNumForName.
	 * Logs an error if not found.
	 * @param Name - Lump name
	 * @return Global lump index, or -1 with error log if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 GetNumForName(const FString& Name) const;

	/**
	 * Get the size of a lump by global index. Equivalent to W_LumpLength.
	 * @param LumpNum - Global lump index
	 * @return Size in bytes, or 0 if invalid
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 GetLumpSize(int32 LumpNum) const;

	/**
	 * Read a lump by global index. Equivalent to W_ReadLump.
	 * @param LumpNum - Global lump index
	 * @param OutData - Output byte array
	 * @return true if read successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	bool ReadLump(int32 LumpNum, TArray<uint8>& OutData);

	/**
	 * Read and cache a lump by global index. Equivalent to W_CacheLumpNum.
	 * @param LumpNum - Global lump index
	 * @return Reference to cached data (empty if invalid)
	 */
	const TArray<uint8>& CacheLumpNum(int32 LumpNum);

	/**
	 * Read and cache a lump by name. Equivalent to W_CacheLumpName.
	 * @param Name - Lump name
	 * @return Reference to cached data (empty if not found)
	 */
	const TArray<uint8>& CacheLumpName(const FString& Name);

	/**
	 * Get the name of a lump by global index.
	 * @param LumpNum - Global lump index
	 * @return Lump name, or empty string if invalid
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	FString GetLumpName(int32 LumpNum) const;

	/** @return Total number of lumps across all loaded WADs */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 GetNumLumps() const;

	// ========================================================================
	// WAD Validation & Game Mode Detection
	// ========================================================================

	/**
	 * Validate a WAD file at the given path without fully loading it.
	 * Checks the header for valid IWAD/PWAD identification.
	 * @param Path - Path to the WAD file
	 * @param OutWadType - Output: the type of WAD (IWAD/PWAD)
	 * @return true if the file is a valid WAD
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	static bool ValidateWadFile(const FString& Path, EWadType& OutWadType);

	/**
	 * Detect the game mode based on currently loaded WADs.
	 * Checks for specific lumps to determine if this is shareware, registered,
	 * commercial, or retail DOOM.
	 *
	 * - Shareware: Only E1M1 present (DOOM1 shareware)
	 * - Registered: E1M1-E3M1 present (DOOM1 registered)
	 * - Retail: E1M1-E4M1 present (Ultimate DOOM)
	 * - Commercial: MAP01 present (DOOM2)
	 *
	 * @return Detected game mode
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	EDoomGameMode DetectGameMode() const;

	/** @return true if an IWAD has been loaded */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	bool HasIWAD() const;

	/** @return Number of loaded WAD files */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 GetNumWadFiles() const { return WadFiles.Num(); }

	/** Flush all cached lump data across all WADs */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	void FlushAllCaches();

	/** Close all WADs and reset the manager */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	void Shutdown();

private:
	/**
	 * Resolve a global lump index to a WAD file index and local lump index.
	 * @param GlobalLumpNum - The global lump index
	 * @param OutWadIndex - Output: index into WadFiles array
	 * @param OutLocalLumpNum - Output: local lump index within that WAD
	 * @return true if the global index was valid
	 */
	bool ResolveGlobalLumpIndex(int32 GlobalLumpNum, int32& OutWadIndex, int32& OutLocalLumpNum) const;

	/** Rebuild the combined directory and name hash after adding a file */
	void RebuildCombinedDirectory();

private:
	/** All loaded WAD files, in order (IWAD typically first, then PWADs) */
	UPROPERTY()
	TArray<TObjectPtr<UWadFile>> WadFiles;

	/**
	 * Combined lump directory across all WADs.
	 * Each entry stores a WAD file index and local lump index.
	 */
	struct FCombinedLumpEntry
	{
		int32 WadIndex = 0;
		int32 LocalLumpIndex = 0;
	};
	TArray<FCombinedLumpEntry> CombinedDirectory;

	/** Combined name hash: uppercase name -> global lump index */
	TMultiMap<FString, int32> CombinedNameHash;

	/** Empty data returned for invalid requests */
	static const TArray<uint8> EmptyData;
};
