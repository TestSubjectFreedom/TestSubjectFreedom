#pragma once

#include "CoreMinimal.h"
#include "VoxelColorPalette.h"

class FVoxLoader
{
public:
	static bool LoadVoxelFromFile(const FString& FilePath, TArray<FVector>& OutVoxelPositions, TArray<FLinearColor>& OutVoxelColors,
		FVoxelColorPalette& ColorPalette, float VoxelSize);

private:
	static bool ProcessMainChunk(TUniquePtr<FArchive>& FileArchive, int64 MainChunkSize, TArray<FVector>& OutVoxelPositions,
		TArray<FLinearColor>& OutVoxelColors, FVoxelColorPalette& ColorPalette, float VoxelSize);
	static bool ProcessXYZIChunk(TUniquePtr<FArchive>& FileArchive, TArray<FVector>& OutVoxelPositions,
		TArray<FLinearColor>& OutVoxelColors, FVoxelColorPalette& ColorPalette, float VoxelSize);
	static bool ProcessRGBAChunk(TUniquePtr<FArchive>& FileArchive, FVoxelColorPalette& ColorPalette);
	static bool SkipChunk(TUniquePtr<FArchive>& FileArchive, int32 ContentSize, int32 ChildrenSize);
};
