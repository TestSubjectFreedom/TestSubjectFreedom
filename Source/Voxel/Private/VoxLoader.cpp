#include "VoxLoader.h"

bool FVoxLoader::LoadVoxelFromFile(const FString& FilePath, TArray<FVector>& OutVoxelPositions,
	TArray<FLinearColor>& OutVoxelColors, FVoxelColorPalette& ColorPalette, float VoxelSize)
{
	IFileManager& FileManager = IFileManager::Get();
	TUniquePtr<FArchive> FileArchive(FileManager.CreateFileReader(*FilePath));
	if (!FileArchive || FileArchive->TotalSize() == 0)
	{
		return false;
	}

	char Header[4];
	FileArchive->Serialize(Header, sizeof(Header));
	if (FMemory::Memcmp(Header, "VOX ", 4) != 0)
	{
		return false;
	}

	int32 Version;
	FileArchive->Serialize(&Version, sizeof(Version));

	while (FileArchive->Tell() < FileArchive->TotalSize())
	{
		char ChunkId[5] = {0};
		FileArchive->Serialize(ChunkId, 4);

		int32 ContentSize, ChildrenSize;
		FileArchive->Serialize(&ContentSize, sizeof(ContentSize));
		FileArchive->Serialize(&ChildrenSize, sizeof(ChildrenSize));

		if (FMemory::Memcmp(ChunkId, "MAIN", 4) == 0)
		{
			if (!ProcessMainChunk(FileArchive, ChildrenSize, OutVoxelPositions, OutVoxelColors, ColorPalette, VoxelSize))
			{
				return false;
			}
		}
		else
		{
			if (!SkipChunk(FileArchive, ContentSize, ChildrenSize))
			{
				return false;
			}
		}
	}

	return OutVoxelPositions.Num() > 0 && OutVoxelColors.Num() == OutVoxelPositions.Num();
}

bool FVoxLoader::ProcessMainChunk(TUniquePtr<FArchive>& FileArchive, int64 MainChunkSize, TArray<FVector>& OutVoxelPositions,
	TArray<FLinearColor>& OutVoxelColors, FVoxelColorPalette& ColorPalette, float VoxelSize)
{
	int64 MainChunkEnd = FileArchive->Tell() + MainChunkSize;

	while (FileArchive->Tell() < MainChunkEnd)
	{
		char SubChunkId[5] = {0};
		FileArchive->Serialize(SubChunkId, 4);

		int32 SubContentSize, SubChildrenSize;
		FileArchive->Serialize(&SubContentSize, sizeof(SubContentSize));
		FileArchive->Serialize(&SubChildrenSize, sizeof(SubChildrenSize));

		if (FMemory::Memcmp(SubChunkId, "RGBA", 4) == 0)
		{
			if (!ProcessRGBAChunk(FileArchive, ColorPalette))
			{
				return false;
			}
		}
		else
		{
			if (!SkipChunk(FileArchive, SubContentSize, SubChildrenSize))
			{
				return false;
			}
		}
	}

	FileArchive->Seek(MainChunkEnd - MainChunkSize);

	while (FileArchive->Tell() < MainChunkEnd)
	{
		char SubChunkId[5] = {0};
		FileArchive->Serialize(SubChunkId, 4);

		int32 SubContentSize, SubChildrenSize;
		FileArchive->Serialize(&SubContentSize, sizeof(SubContentSize));
		FileArchive->Serialize(&SubChildrenSize, sizeof(SubChildrenSize));

		if (FMemory::Memcmp(SubChunkId, "XYZI", 4) == 0)
		{
			if (!ProcessXYZIChunk(FileArchive, OutVoxelPositions, OutVoxelColors, ColorPalette, VoxelSize))
			{
				return false;
			}
		}
		else
		{
			if (!SkipChunk(FileArchive, SubContentSize, SubChildrenSize))
			{
				return false;
			}
		}
	}
	return true;
}

bool FVoxLoader::ProcessXYZIChunk(TUniquePtr<FArchive>& FileArchive, TArray<FVector>& OutVoxelPositions,
	TArray<FLinearColor>& OutVoxelColors, FVoxelColorPalette& ColorPalette, float VoxelSize)
{
	int32 NumVoxels;
	FileArchive->Serialize(&NumVoxels, sizeof(NumVoxels));

	if (NumVoxels <= 0 || FileArchive->Tell() + NumVoxels * 4 > FileArchive->TotalSize())
	{
		return false;
	}

	for (int32 i = 0; i < NumVoxels; i++)
	{
		uint8 X, Y, Z, ColorIndex;
		FileArchive->Serialize(&X, sizeof(X));
		FileArchive->Serialize(&Y, sizeof(Y));
		FileArchive->Serialize(&Z, sizeof(Z));
		FileArchive->Serialize(&ColorIndex, sizeof(ColorIndex));

		OutVoxelPositions.Add(FVector(X * VoxelSize, Y * VoxelSize, Z * VoxelSize));
		OutVoxelColors.Add(ColorPalette.GetColor(ColorIndex));
	}
	return true;
}

bool FVoxLoader::ProcessRGBAChunk(TUniquePtr<FArchive>& FileArchive, FVoxelColorPalette& ColorPalette)
{
	ColorPalette.InitializeFromFileRGBA(FileArchive);
	return true;
}

bool FVoxLoader::SkipChunk(TUniquePtr<FArchive>& FileArchive, int32 ContentSize, int32 ChildrenSize)
{
	int64 SkipAmount = static_cast<int64>(ContentSize) + static_cast<int64>(ChildrenSize);
	if (FileArchive->Tell() + SkipAmount > FileArchive->TotalSize())
	{
		return false;
	}
	FileArchive->Seek(FileArchive->Tell() + SkipAmount);
	return true;
}
