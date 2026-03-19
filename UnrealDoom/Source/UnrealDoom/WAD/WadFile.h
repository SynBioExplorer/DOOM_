#pragma once

#include "CoreMinimal.h"
#include "WadTypes.h"
#include "WadFile.generated.h"

/**
 * UWadFile - Manages a single WAD file (IWAD or PWAD).
 *
 * Port of the WAD loading system from linuxdoom-1.10 w_wad.c.
 * Handles reading the WAD header, building the lump directory,
 * and providing cached access to lump data.
 *
 * Key differences from the original:
 * - Uses UE5 file I/O (IPlatformFile) instead of POSIX open/read/lseek
 * - Uses TMap for lump caching instead of Z_Malloc zone memory
 * - Uses TMultiMap hash table for fast name lookups instead of linear scan
 * - Supports proper FString-based lump name queries
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UWadFile : public UObject
{
	GENERATED_BODY()

public:
	UWadFile();

	/**
	 * Open and parse a WAD file, reading its header and lump directory.
	 * Equivalent to W_AddFile for a single WAD.
	 * @param Path - Absolute or relative filesystem path to the .wad file
	 * @return true if the WAD was loaded successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	bool OpenWadFile(const FString& Path);

	/** Close and release all resources associated with this WAD */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	void Close();

	/** @return Total number of lumps in this WAD */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 GetNumLumps() const;

	/**
	 * Find a lump by name. Searches backwards so later entries override earlier ones.
	 * Case insensitive, max 8 characters. Equivalent to W_CheckNumForName.
	 * @param Name - Lump name to search for
	 * @return Lump index, or -1 if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 FindLump(const FString& Name) const;

	/**
	 * Get the size of a lump in bytes. Equivalent to W_LumpLength.
	 * @param LumpNum - Index of the lump
	 * @return Size in bytes, or 0 if invalid index
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	int32 GetLumpSize(int32 LumpNum) const;

	/**
	 * Read a lump's data into a byte array. Equivalent to W_ReadLump.
	 * @param LumpNum - Index of the lump to read
	 * @param OutData - Output byte array, resized to fit the lump data
	 * @return true if the lump was read successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	bool ReadLump(int32 LumpNum, TArray<uint8>& OutData);

	/**
	 * Get the name of a lump by index.
	 * @param LumpNum - Index of the lump
	 * @return Lump name as FString, or empty string if invalid
	 */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	FString GetLumpName(int32 LumpNum) const;

	/**
	 * Read and cache a lump by index. Subsequent calls return the cached data.
	 * Equivalent to W_CacheLumpNum.
	 * @param LumpNum - Index of the lump
	 * @return Reference to the cached byte array (empty if invalid)
	 */
	const TArray<uint8>& CacheLumpNum(int32 LumpNum);

	/**
	 * Read and cache a lump by name. Equivalent to W_CacheLumpName.
	 * @param Name - Lump name to find and cache
	 * @return Reference to the cached byte array (empty if not found)
	 */
	const TArray<uint8>& CacheLumpName(const FString& Name);

	/** @return The type of this WAD (IWAD or PWAD) */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	EWadType GetWadType() const { return WadType; }

	/** @return true if the WAD file is currently open and valid */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	bool IsValid() const { return bIsValid; }

	/** @return The file path this WAD was loaded from */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	FString GetFilePath() const { return FilePath; }

	/** @return Direct access to the lump info array */
	const TArray<FWadLumpInfo>& GetLumpInfoArray() const { return LumpInfoArray; }

	/** Flush all cached lump data from memory */
	UFUNCTION(BlueprintCallable, Category = "WAD")
	void FlushCache();

private:
	/** Parse the WAD header from raw file data */
	bool ParseHeader(IFileHandle* FileHandle);

	/** Read the lump directory from the file */
	bool ReadDirectory(IFileHandle* FileHandle);

	/** Build the name hash table for fast lookups */
	void BuildNameHashTable();

	/** Normalize a lump name: uppercase, trimmed, max 8 chars */
	static FString NormalizeLumpName(const FString& Name);

private:
	/** Whether this WAD is valid and open */
	bool bIsValid = false;

	/** IWAD or PWAD */
	UPROPERTY()
	EWadType WadType = EWadType::Invalid;

	/** Path to the WAD file on disk */
	UPROPERTY()
	FString FilePath;

	/** Parsed header info */
	FWadHeader Header;

	/** Array of all lump info entries (the directory) */
	TArray<FWadLumpInfo> LumpInfoArray;

	/** Hash map from uppercase lump name -> array of lump indices (for fast lookup) */
	TMultiMap<FString, int32> NameHashTable;

	/** Lump data cache: lump index -> cached byte data */
	TMap<int32, TArray<uint8>> LumpCache;

	/** Empty array returned for invalid requests */
	static const TArray<uint8> EmptyData;
};
