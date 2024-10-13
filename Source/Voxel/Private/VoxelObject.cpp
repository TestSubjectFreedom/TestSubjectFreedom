#include "VoxelObject.h"

#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "VoxLoader.h"

AVoxelObject::AVoxelObject()
{
	PrimaryActorTick.bCanEverTick = true;

	VoxelMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VoxelMesh"));
	RootComponent = VoxelMesh;
}

void AVoxelObject::BeginPlay()
{
	Super::BeginPlay();
}

void AVoxelObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void AVoxelObject::LoadModelInEditor()
{
	if (!ModelFilePath.FilePath.IsEmpty())
	{
		LoadVoxelFromFile();
	}
}
#endif

void AVoxelObject::LoadVoxelFromFile()
{
	VoxelMesh->ClearAllMeshSections();

	const FString FilePath = ModelFilePath.FilePath;

	TArray<FVector> VoxelPositions;
	TArray<FLinearColor> VoxelColors;
	if (FVoxLoader::LoadVoxelFromFile(FilePath, VoxelPositions, VoxelColors, ColorPalette, VoxelSize))
	{
		TSet<FString> LoggedColors;
		CreateGreedyVoxelMesh(VoxelPositions, VoxelColors);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load voxel data from file."));
	}
}

void AVoxelObject::CreateGreedyVoxelMesh(const TArray<FVector>& VoxelPositions, const TArray<FLinearColor>& VoxelColors)
{
	constexpr int32 GridSizeX = 100;
	constexpr int32 GridSizeY = 100;
	constexpr int32 GridSizeZ = 100;

	struct FVoxel
	{
		bool IsSolid = false;
		FLinearColor Color;
	};

	FVoxel VoxelGrid[GridSizeX][GridSizeY][GridSizeZ];

	for (int32 i = 0; i < VoxelPositions.Num(); ++i)
	{
		FVector VoxelPosition = VoxelPositions[i];
		FLinearColor VoxelColor = VoxelColors[i];

		int32 X = static_cast<int32>(VoxelPosition.X / VoxelSize);
		int32 Y = static_cast<int32>(VoxelPosition.Y / VoxelSize);
		int32 Z = static_cast<int32>(VoxelPosition.Z / VoxelSize);

		if (X >= 0 && X < GridSizeX && Y >= 0 && Y < GridSizeY && Z >= 0 && Z < GridSizeZ)
		{
			VoxelGrid[X][Y][Z].IsSolid = true;
			VoxelGrid[X][Y][Z].Color = VoxelColor;
		}
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FLinearColor> VertexColors;
	TArray<FVector> Normals;

	auto AddQuad = [&](FVector V0, FVector V1, FVector V2, FVector V3, FLinearColor Color, FVector Normal)
	{
		int32 VertexIndex = Vertices.Num();

		Vertices.Add(V0);
		Vertices.Add(V1);
		Vertices.Add(V2);
		Vertices.Add(V3);

		Triangles.Append({VertexIndex, VertexIndex + 1, VertexIndex + 2});
		Triangles.Append({VertexIndex, VertexIndex + 2, VertexIndex + 3});

		VertexColors.Append({Color, Color, Color, Color});

		Normals.Append({Normal, Normal, Normal, Normal});
	};

	for (int32 z = 0; z < GridSizeZ; ++z)
	{
		for (int32 y = 0; y < GridSizeY; ++y)
		{
			for (int32 x = 0; x < GridSizeX; ++x)
			{
				if (!VoxelGrid[x][y][z].IsSolid)
				{
					continue;
				}

				FLinearColor CurrentColor = VoxelGrid[x][y][z].Color;

				if (z == GridSizeZ - 1 || !VoxelGrid[x][y][z + 1].IsSolid)
				{
					FVector V0 = FVector(x * VoxelSize, y * VoxelSize, (z + 1) * VoxelSize);
					FVector V1 = FVector((x + 1) * VoxelSize, y * VoxelSize, (z + 1) * VoxelSize);
					FVector V2 = FVector((x + 1) * VoxelSize, (y + 1) * VoxelSize, (z + 1) * VoxelSize);
					FVector V3 = FVector(x * VoxelSize, (y + 1) * VoxelSize, (z + 1) * VoxelSize);
					FVector Normal = FVector(0, 0, 1);

					AddQuad(V0, V3, V2, V1, CurrentColor, Normal);
				}

				if (z == 0 || !VoxelGrid[x][y][z - 1].IsSolid)
				{
					FVector V0 = FVector(x * VoxelSize, y * VoxelSize, z * VoxelSize);
					FVector V1 = FVector((x + 1) * VoxelSize, y * VoxelSize, z * VoxelSize);
					FVector V2 = FVector((x + 1) * VoxelSize, (y + 1) * VoxelSize, z * VoxelSize);
					FVector V3 = FVector(x * VoxelSize, (y + 1) * VoxelSize, z * VoxelSize);
					FVector Normal = FVector(0, 0, -1);

					AddQuad(V0, V1, V2, V3, CurrentColor, Normal);
				}

				if (y == GridSizeY - 1 || !VoxelGrid[x][y + 1][z].IsSolid)
				{
					FVector V0 = FVector(x * VoxelSize, (y + 1) * VoxelSize, z * VoxelSize);
					FVector V1 = FVector((x + 1) * VoxelSize, (y + 1) * VoxelSize, z * VoxelSize);
					FVector V2 = FVector((x + 1) * VoxelSize, (y + 1) * VoxelSize, (z + 1) * VoxelSize);
					FVector V3 = FVector(x * VoxelSize, (y + 1) * VoxelSize, (z + 1) * VoxelSize);
					FVector Normal = FVector(0, 1, 0);

					AddQuad(V0, V1, V2, V3, CurrentColor, Normal);
				}

				if (y == 0 || !VoxelGrid[x][y - 1][z].IsSolid)
				{
					FVector V0 = FVector(x * VoxelSize, y * VoxelSize, z * VoxelSize);
					FVector V1 = FVector(x * VoxelSize, y * VoxelSize, (z + 1) * VoxelSize);
					FVector V2 = FVector((x + 1) * VoxelSize, y * VoxelSize, (z + 1) * VoxelSize);
					FVector V3 = FVector((x + 1) * VoxelSize, y * VoxelSize, z * VoxelSize);
					FVector Normal = FVector(0, -1, 0);

					AddQuad(V0, V1, V2, V3, CurrentColor, Normal);
				}

				if (x == GridSizeX - 1 || !VoxelGrid[x + 1][y][z].IsSolid)
				{
					FVector V0 = FVector((x + 1) * VoxelSize, y * VoxelSize, z * VoxelSize);
					FVector V1 = FVector((x + 1) * VoxelSize, y * VoxelSize, (z + 1) * VoxelSize);
					FVector V2 = FVector((x + 1) * VoxelSize, (y + 1) * VoxelSize, (z + 1) * VoxelSize);
					FVector V3 = FVector((x + 1) * VoxelSize, (y + 1) * VoxelSize, z * VoxelSize);
					FVector Normal = FVector(1, 0, 0);

					AddQuad(V0, V1, V2, V3, CurrentColor, Normal);
				}

				if (x == 0 || !VoxelGrid[x - 1][y][z].IsSolid)
				{
					FVector V0 = FVector(x * VoxelSize, y * VoxelSize, z * VoxelSize);
					FVector V1 = FVector(x * VoxelSize, (y + 1) * VoxelSize, z * VoxelSize);
					FVector V2 = FVector(x * VoxelSize, (y + 1) * VoxelSize, (z + 1) * VoxelSize);
					FVector V3 = FVector(x * VoxelSize, y * VoxelSize, (z + 1) * VoxelSize);
					FVector Normal = FVector(-1, 0, 0);

					AddQuad(V0, V1, V2, V3, CurrentColor, Normal);
				}
			}
		}
	}

	VoxelMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, {}, VertexColors, {}, true);
}
