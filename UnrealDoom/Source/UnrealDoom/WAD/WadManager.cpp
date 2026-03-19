#include "WadManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

// ============================================================================
// Static members
// ============================================================================

const TArray<uint8> UWadManager::EmptyData;

// ============================================================================
// Construction
// ============================================================================

UWadManager::UWadManager()
{
}

// ============================================================================
// File Management
// ============================================================================

bool UWadManager::AddFile(const FString& Path)
{
	// Create a new WAD file object
	UWadFile* NewWad = NewObject<UWadFile>(this);

	if (!NewWad->OpenWadFile(Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadManager::AddFile: Could not open %s"), *Path);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("UWadManager::AddFile: Adding %s (%s, %d lumps)"),
		*Path,
		NewWad->GetWadType() == EWadType::IWAD ? TEXT("IWAD") : TEXT("PWAD"),
		NewWad->GetNumLumps());

	WadFiles.Add(NewWad);

	// Rebuild the combined directory to include the new WAD
	RebuildCombinedDirectory();

	return true;
}

bool UWadManager::InitMultipleFiles(const TArray<FString>& FileNames)
{
	// Close any previously loaded WADs
	Shutdown();

	// Open all the files, load headers, and count lumps
	// All files are optional, but at least one must be found.
	// Matches original W_InitMultipleFiles behavior.
	int32 SuccessCount = 0;

	for (const FString& FileName : FileNames)
	{
		if (FileName.IsEmpty())
		{
			continue;
		}

		UWadFile* NewWad = NewObject<UWadFile>(this);

		if (NewWad->OpenWadFile(FileName))
		{
			WadFiles.Add(NewWad);
			SuccessCount++;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UWadManager::InitMultipleFiles: Could not open %s"), *FileName);
		}
	}

	if (SuccessCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("UWadManager::InitMultipleFiles: No files found!"));
		return false;
	}

	// Build the combined directory across all loaded WADs
	RebuildCombinedDirectory();

	UE_LOG(LogTemp, Log, TEXT("UWadManager::InitMultipleFiles: Loaded %d files, %d total lumps"),
		SuccessCount, CombinedDirectory.Num());

	return true;
}

// ============================================================================
// Combined Directory Management
// ============================================================================

void UWadManager::RebuildCombinedDirectory()
{
	CombinedDirectory.Empty();
	CombinedNameHash.Empty();

	// Build combined directory: iterate WADs in order, appending all lumps.
	// This means later WAD entries get higher global indices.
	// When searching by name backwards, later entries (PWADs) win.
	for (int32 WadIdx = 0; WadIdx < WadFiles.Num(); ++WadIdx)
	{
		const UWadFile* Wad = WadFiles[WadIdx];
		if (!Wad || !Wad->IsValid())
		{
			continue;
		}

		const int32 NumLumps = Wad->GetNumLumps();
		for (int32 LocalIdx = 0; LocalIdx < NumLumps; ++LocalIdx)
		{
			const int32 GlobalIdx = CombinedDirectory.Num();

			FCombinedLumpEntry Entry;
			Entry.WadIndex = WadIdx;
			Entry.LocalLumpIndex = LocalIdx;
			CombinedDirectory.Add(Entry);

			// Add to name hash
			FString LumpName = Wad->GetLumpName(LocalIdx).ToUpper();
			if (!LumpName.IsEmpty())
			{
				CombinedNameHash.Add(LumpName, GlobalIdx);
			}
		}
	}
}

bool UWadManager::ResolveGlobalLumpIndex(int32 GlobalLumpNum, int32& OutWadIndex, int32& OutLocalLumpNum) const
{
	if (GlobalLumpNum < 0 || GlobalLumpNum >= CombinedDirectory.Num())
	{
		return false;
	}

	const FCombinedLumpEntry& Entry = CombinedDirectory[GlobalLumpNum];
	OutWadIndex = Entry.WadIndex;
	OutLocalLumpNum = Entry.LocalLumpIndex;
	return true;
}

// ============================================================================
// Lump Access
// ============================================================================

int32 UWadManager::FindLump(const FString& Name) const
{
	const FString NormalizedName = Name.ToUpper().Left(8).TrimStartAndEnd();

	// Find all entries with this name, return the highest index
	// (scan backwards so patch lump files take precedence - matching original behavior)
	TArray<int32> Indices;
	CombinedNameHash.MultiFind(NormalizedName, Indices);

	if (Indices.Num() == 0)
	{
		return -1;
	}

	int32 BestIndex = -1;
	for (int32 Idx : Indices)
	{
		if (Idx > BestIndex)
		{
			BestIndex = Idx;
		}
	}

	return BestIndex;
}

int32 UWadManager::GetNumForName(const FString& Name) const
{
	const int32 LumpNum = FindLump(Name);

	if (LumpNum == -1)
	{
		UE_LOG(LogTemp, Error, TEXT("UWadManager::GetNumForName: '%s' not found!"), *Name);
	}

	return LumpNum;
}

int32 UWadManager::GetLumpSize(int32 LumpNum) const
{
	int32 WadIndex, LocalIndex;
	if (!ResolveGlobalLumpIndex(LumpNum, WadIndex, LocalIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadManager::GetLumpSize: Invalid lump %d >= %d"), LumpNum, CombinedDirectory.Num());
		return 0;
	}

	return WadFiles[WadIndex]->GetLumpSize(LocalIndex);
}

bool UWadManager::ReadLump(int32 LumpNum, TArray<uint8>& OutData)
{
	int32 WadIndex, LocalIndex;
	if (!ResolveGlobalLumpIndex(LumpNum, WadIndex, LocalIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadManager::ReadLump: Invalid lump %d >= %d"), LumpNum, CombinedDirectory.Num());
		return false;
	}

	return WadFiles[WadIndex]->ReadLump(LocalIndex, OutData);
}

const TArray<uint8>& UWadManager::CacheLumpNum(int32 LumpNum)
{
	int32 WadIndex, LocalIndex;
	if (!ResolveGlobalLumpIndex(LumpNum, WadIndex, LocalIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadManager::CacheLumpNum: Invalid lump %d >= %d"), LumpNum, CombinedDirectory.Num());
		return EmptyData;
	}

	return WadFiles[WadIndex]->CacheLumpNum(LocalIndex);
}

const TArray<uint8>& UWadManager::CacheLumpName(const FString& Name)
{
	const int32 LumpNum = FindLump(Name);
	if (LumpNum == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadManager::CacheLumpName: '%s' not found"), *Name);
		return EmptyData;
	}

	return CacheLumpNum(LumpNum);
}

FString UWadManager::GetLumpName(int32 LumpNum) const
{
	int32 WadIndex, LocalIndex;
	if (!ResolveGlobalLumpIndex(LumpNum, WadIndex, LocalIndex))
	{
		return FString();
	}

	return WadFiles[WadIndex]->GetLumpName(LocalIndex);
}

int32 UWadManager::GetNumLumps() const
{
	return CombinedDirectory.Num();
}

// ============================================================================
// WAD Validation & Game Mode Detection
// ============================================================================

bool UWadManager::ValidateWadFile(const FString& Path, EWadType& OutWadType)
{
	OutWadType = EWadType::Invalid;

	const FString FullPath = FPaths::ConvertRelativePathToFull(Path);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.FileExists(*FullPath))
	{
		return false;
	}

	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FullPath));
	if (!FileHandle)
	{
		return false;
	}

	// Read the 4-byte identification
	char Identification[4];
	if (!FileHandle->Read(reinterpret_cast<uint8*>(Identification), 4))
	{
		return false;
	}

	if (FMemory::Memcmp(Identification, "IWAD", 4) == 0)
	{
		OutWadType = EWadType::IWAD;
		return true;
	}
	else if (FMemory::Memcmp(Identification, "PWAD", 4) == 0)
	{
		OutWadType = EWadType::PWAD;
		return true;
	}

	return false;
}

EDoomGameMode UWadManager::DetectGameMode() const
{
	if (CombinedDirectory.Num() == 0)
	{
		return EDoomGameMode::Indetermined;
	}

	// Check for DOOM2 / commercial: look for MAP01
	const bool bHasMAP01 = FindLump(TEXT("MAP01")) != -1;
	if (bHasMAP01)
	{
		return EDoomGameMode::Commercial;
	}

	// Check for DOOM1 episodes
	const bool bHasE1M1 = FindLump(TEXT("E1M1")) != -1;
	const bool bHasE2M1 = FindLump(TEXT("E2M1")) != -1;
	const bool bHasE3M1 = FindLump(TEXT("E3M1")) != -1;
	const bool bHasE4M1 = FindLump(TEXT("E4M1")) != -1;

	if (bHasE4M1)
	{
		// Ultimate DOOM (retail) - has Episode 4
		return EDoomGameMode::Retail;
	}

	if (bHasE3M1)
	{
		// DOOM1 registered - Episodes 1-3
		return EDoomGameMode::Registered;
	}

	if (bHasE1M1)
	{
		// DOOM1 shareware - Episode 1 only
		return EDoomGameMode::Shareware;
	}

	return EDoomGameMode::Indetermined;
}

bool UWadManager::HasIWAD() const
{
	for (const TObjectPtr<UWadFile>& Wad : WadFiles)
	{
		if (Wad && Wad->GetWadType() == EWadType::IWAD)
		{
			return true;
		}
	}
	return false;
}

// ============================================================================
// Cache & Lifecycle Management
// ============================================================================

void UWadManager::FlushAllCaches()
{
	for (TObjectPtr<UWadFile>& Wad : WadFiles)
	{
		if (Wad)
		{
			Wad->FlushCache();
		}
	}
}

void UWadManager::Shutdown()
{
	for (TObjectPtr<UWadFile>& Wad : WadFiles)
	{
		if (Wad)
		{
			Wad->Close();
		}
	}

	WadFiles.Empty();
	CombinedDirectory.Empty();
	CombinedNameHash.Empty();
}
