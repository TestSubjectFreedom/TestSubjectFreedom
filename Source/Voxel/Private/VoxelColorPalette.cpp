#include "VoxelColorPalette.h"

FVoxelColorPalette::FVoxelColorPalette()
{
	Colors.SetNum(256);
}

void FVoxelColorPalette::InitializeFromFileRGBA(TUniquePtr<FArchive>& FileArchive)
{
	Colors.Empty();

	for (int32 i = 0; i < 256; ++i)
	{
		uint8 R, G, B, A;
		FileArchive->Serialize(&R, sizeof(R));
		FileArchive->Serialize(&G, sizeof(G));
		FileArchive->Serialize(&B, sizeof(B));
		FileArchive->Serialize(&A, sizeof(A));
		Colors.Add(FLinearColor(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f));
	}
}

FLinearColor FVoxelColorPalette::GetColor(const uint8 ColorIndex) const
{
	if (Colors.IsValidIndex(ColorIndex))
	{
		return Colors[ColorIndex];
	}
	return FLinearColor::White;
}
