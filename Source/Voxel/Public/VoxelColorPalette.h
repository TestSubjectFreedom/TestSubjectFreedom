#pragma once

#include "CoreMinimal.h"

class VOXEL_API FVoxelColorPalette
{
public:
	FVoxelColorPalette();

	void InitializeFromFileRGBA(TUniquePtr<FArchive>& FileArchive);

	FLinearColor GetColor(const uint8 ColorIndex) const;

private:
	TArray<FLinearColor> Colors;
};
