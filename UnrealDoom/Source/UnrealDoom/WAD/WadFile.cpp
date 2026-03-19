#include "WadFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

// ============================================================================
// Static members
// ============================================================================

const TArray<uint8> UWadFile::EmptyData;

// ============================================================================
// Construction
// ============================================================================

UWadFile::UWadFile()
{
	FMemory::Memzero(&Header, sizeof(FWadHeader));
}

// ============================================================================
// File Operations
// ============================================================================

bool UWadFile::OpenWadFile(const FString& Path)
{
	Close();

	// Resolve the full path
	FilePath = FPaths::ConvertRelativePathToFull(Path);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::OpenWadFile: File not found: %s"), *FilePath);
		return false;
	}

	// Open file for reading
	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FilePath));
	if (!FileHandle)
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::OpenWadFile: Could not open file: %s"), *FilePath);
		return false;
	}

	// Check if this is a WAD file by extension
	const FString Extension = FPaths::GetExtension(FilePath).ToLower();

	if (Extension == TEXT("wad"))
	{
		// Parse WAD header and directory
		if (!ParseHeader(FileHandle.Get()))
		{
			UE_LOG(LogTemp, Warning, TEXT("UWadFile::OpenWadFile: Failed to parse WAD header: %s"), *FilePath);
			return false;
		}

		if (!ReadDirectory(FileHandle.Get()))
		{
			UE_LOG(LogTemp, Warning, TEXT("UWadFile::OpenWadFile: Failed to read WAD directory: %s"), *FilePath);
			return false;
		}
	}
	else
	{
		// Single lump file - treat the whole file as one lump
		// Extract base filename as the lump name (matching original W_AddFile behavior)
		const int64 FileSize = FileHandle->Size();

		FWadLumpInfo SingleLump;
		SingleLump.FilePos = 0;
		SingleLump.Size = static_cast<int32>(FileSize);
		SingleLump.WadFileIndex = 0;

		// Extract base name (without extension), uppercase, max 8 chars
		FString BaseName = FPaths::GetBaseFilename(FilePath);
		SingleLump.SetName(BaseName);

		LumpInfoArray.Add(SingleLump);
		WadType = EWadType::PWAD; // Treat single files as patch data
	}

	// Build the hash table for fast name lookups
	BuildNameHashTable();

	bIsValid = true;
	UE_LOG(LogTemp, Log, TEXT("UWadFile::OpenWadFile: Loaded %s (%s, %d lumps)"),
		*FilePath,
		WadType == EWadType::IWAD ? TEXT("IWAD") : TEXT("PWAD"),
		LumpInfoArray.Num());

	return true;
}

void UWadFile::Close()
{
	bIsValid = false;
	WadType = EWadType::Invalid;
	FilePath.Empty();
	FMemory::Memzero(&Header, sizeof(FWadHeader));
	LumpInfoArray.Empty();
	NameHashTable.Empty();
	LumpCache.Empty();
}

// ============================================================================
// Header & Directory Parsing
// ============================================================================

bool UWadFile::ParseHeader(IFileHandle* FileHandle)
{
	if (!FileHandle)
	{
		return false;
	}

	// Seek to beginning
	FileHandle->Seek(0);

	// Read the raw 12-byte header: 4 chars + 2 ints (little-endian on disk)
	if (!FileHandle->Read(reinterpret_cast<uint8*>(&Header), sizeof(FWadHeader)))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::ParseHeader: Could not read header"));
		return false;
	}

	// Validate identification - must be "IWAD" or "PWAD"
	if (FMemory::Memcmp(Header.Identification, "IWAD", 4) == 0)
	{
		WadType = EWadType::IWAD;
	}
	else if (FMemory::Memcmp(Header.Identification, "PWAD", 4) == 0)
	{
		WadType = EWadType::PWAD;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::ParseHeader: Invalid WAD identification in %s"), *FilePath);
		WadType = EWadType::Invalid;
		return false;
	}

	// WAD files store integers in little-endian format.
	// UE5 platforms are little-endian, so no byte-swap needed for x86/x64/ARM64.
	// The original code used LONG() macro for endian conversion.

	if (Header.NumLumps < 0 || Header.InfoTableOfs < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::ParseHeader: Invalid header values (NumLumps=%d, InfoTableOfs=%d)"),
			Header.NumLumps, Header.InfoTableOfs);
		return false;
	}

	return true;
}

bool UWadFile::ReadDirectory(IFileHandle* FileHandle)
{
	if (!FileHandle || Header.NumLumps <= 0)
	{
		return Header.NumLumps == 0; // Empty WAD is technically valid
	}

	// Seek to the info table offset
	if (!FileHandle->Seek(Header.InfoTableOfs))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::ReadDirectory: Could not seek to directory at offset %d"), Header.InfoTableOfs);
		return false;
	}

	// Read the raw directory entries (filelump_t format: 4 bytes filepos, 4 bytes size, 8 bytes name)
	// Each entry is 16 bytes, matching the original filelump_t struct
	struct FFileLumpRaw
	{
		int32 FilePos;
		int32 Size;
		char Name[8];
	};
	static_assert(sizeof(FFileLumpRaw) == 16, "FFileLumpRaw must be 16 bytes to match WAD format");

	const int32 DirectorySize = Header.NumLumps * sizeof(FFileLumpRaw);
	TArray<uint8> DirectoryData;
	DirectoryData.SetNumUninitialized(DirectorySize);

	if (!FileHandle->Read(DirectoryData.GetData(), DirectorySize))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::ReadDirectory: Could not read directory (%d entries)"), Header.NumLumps);
		return false;
	}

	// Parse each directory entry into our lump info array
	LumpInfoArray.Reserve(Header.NumLumps);
	const FFileLumpRaw* RawEntries = reinterpret_cast<const FFileLumpRaw*>(DirectoryData.GetData());

	for (int32 i = 0; i < Header.NumLumps; ++i)
	{
		const FFileLumpRaw& RawEntry = RawEntries[i];

		FWadLumpInfo LumpInfo;
		LumpInfo.FilePos = RawEntry.FilePos;
		LumpInfo.Size = RawEntry.Size;
		LumpInfo.WadFileIndex = 0;

		// Copy name and convert to uppercase (matching original strupr behavior)
		FMemory::Memcpy(LumpInfo.Name, RawEntry.Name, 8);
		for (int32 c = 0; c < 8; ++c)
		{
			if (LumpInfo.Name[c] >= 'a' && LumpInfo.Name[c] <= 'z')
			{
				LumpInfo.Name[c] = LumpInfo.Name[c] - 'a' + 'A';
			}
		}

		LumpInfoArray.Add(LumpInfo);
	}

	return true;
}

void UWadFile::BuildNameHashTable()
{
	NameHashTable.Empty(LumpInfoArray.Num());

	for (int32 i = 0; i < LumpInfoArray.Num(); ++i)
	{
		FString LumpName = LumpInfoArray[i].GetName();
		if (!LumpName.IsEmpty())
		{
			NameHashTable.Add(LumpName.ToUpper(), i);
		}
	}
}

// ============================================================================
// Lump Queries
// ============================================================================

int32 UWadFile::GetNumLumps() const
{
	return LumpInfoArray.Num();
}

int32 UWadFile::FindLump(const FString& Name) const
{
	const FString NormalizedName = NormalizeLumpName(Name);

	// Find all entries with this name, return the last one
	// (scan backwards so later entries override earlier - matching original W_CheckNumForName)
	TArray<int32> Indices;
	NameHashTable.MultiFind(NormalizedName, Indices);

	if (Indices.Num() == 0)
	{
		return -1;
	}

	// MultiFind returns in insertion order; we want the highest index (last added)
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

int32 UWadFile::GetLumpSize(int32 LumpNum) const
{
	if (LumpNum < 0 || LumpNum >= LumpInfoArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::GetLumpSize: Invalid lump index %d (total: %d)"),
			LumpNum, LumpInfoArray.Num());
		return 0;
	}

	return LumpInfoArray[LumpNum].Size;
}

bool UWadFile::ReadLump(int32 LumpNum, TArray<uint8>& OutData)
{
	if (LumpNum < 0 || LumpNum >= LumpInfoArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::ReadLump: Invalid lump index %d (total: %d)"),
			LumpNum, LumpInfoArray.Num());
		return false;
	}

	const FWadLumpInfo& LumpInfo = LumpInfoArray[LumpNum];

	// Handle zero-size lumps (markers like map labels)
	if (LumpInfo.Size == 0)
	{
		OutData.Empty();
		return true;
	}

	// Open the file for reading
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FilePath));

	if (!FileHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("UWadFile::ReadLump: Could not open WAD file for reading: %s"), *FilePath);
		return false;
	}

	// Seek to lump position and read
	if (!FileHandle->Seek(LumpInfo.FilePos))
	{
		UE_LOG(LogTemp, Error, TEXT("UWadFile::ReadLump: Could not seek to lump %d at offset %d"),
			LumpNum, LumpInfo.FilePos);
		return false;
	}

	OutData.SetNumUninitialized(LumpInfo.Size);

	if (!FileHandle->Read(OutData.GetData(), LumpInfo.Size))
	{
		UE_LOG(LogTemp, Error, TEXT("UWadFile::ReadLump: Failed to read %d bytes for lump %d (%s)"),
			LumpInfo.Size, LumpNum, *LumpInfo.GetName());
		OutData.Empty();
		return false;
	}

	return true;
}

FString UWadFile::GetLumpName(int32 LumpNum) const
{
	if (LumpNum < 0 || LumpNum >= LumpInfoArray.Num())
	{
		return FString();
	}

	return LumpInfoArray[LumpNum].GetName();
}

// ============================================================================
// Caching System
// ============================================================================

const TArray<uint8>& UWadFile::CacheLumpNum(int32 LumpNum)
{
	if (LumpNum < 0 || LumpNum >= LumpInfoArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::CacheLumpNum: Invalid lump index %d"), LumpNum);
		return EmptyData;
	}

	// Check if already cached
	if (TArray<uint8>* Cached = LumpCache.Find(LumpNum))
	{
		return *Cached;
	}

	// Cache miss - read the lump data
	TArray<uint8> Data;
	if (!ReadLump(LumpNum, Data))
	{
		return EmptyData;
	}

	// Store in cache and return reference
	return LumpCache.Add(LumpNum, MoveTemp(Data));
}

const TArray<uint8>& UWadFile::CacheLumpName(const FString& Name)
{
	const int32 LumpNum = FindLump(Name);
	if (LumpNum == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("UWadFile::CacheLumpName: Lump '%s' not found"), *Name);
		return EmptyData;
	}

	return CacheLumpNum(LumpNum);
}

void UWadFile::FlushCache()
{
	LumpCache.Empty();
}

// ============================================================================
// Utility
// ============================================================================

FString UWadFile::NormalizeLumpName(const FString& Name)
{
	return Name.ToUpper().Left(8).TrimStartAndEnd();
}
