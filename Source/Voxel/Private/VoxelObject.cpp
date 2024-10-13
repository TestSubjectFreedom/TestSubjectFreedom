#include "VoxelObject.h"

#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "VoxLoader.h"

AVoxelObject::AVoxelObject()
{
	PrimaryActorTick.bCanEverTick = true;

	VoxelMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VoxelMeshComponent"));
	RootComponent = VoxelMeshComponent;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
		TEXT("Material'/Game/Materials/VoxelMaterial.VoxelMaterial'"));
	if (MaterialFinder.Succeeded())
	{
		VoxelMaterial = MaterialFinder.Object;
		VoxelMeshComponent->SetMaterial(0, VoxelMaterial);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load VoxelMaterial. Please make sure the path is correct."));
	}
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
	const FString FilePath = ModelFilePath.FilePath;

	TArray<FVector> VoxelPositions;
	TArray<FLinearColor> VoxelColors;
	if (FVoxLoader::LoadVoxelFromFile(FilePath, VoxelPositions, VoxelColors, ColorPalette, VoxelSize))
	{
		CreateGreedyVoxelMesh(VoxelPositions, VoxelColors);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load voxel data from file."));
	}
}

void AVoxelObject::CreateGreedyVoxelMesh(const TArray<FVector>& VoxelPositions, const TArray<FLinearColor>& VoxelColors) const
{
	FVector MinVoxel(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector MaxVoxel(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const FVector& VoxelPosition : VoxelPositions)
	{
		MinVoxel.X = FMath::Min(MinVoxel.X, VoxelPosition.X);
		MinVoxel.Y = FMath::Min(MinVoxel.Y, VoxelPosition.Y);
		MinVoxel.Z = FMath::Min(MinVoxel.Z, VoxelPosition.Z);

		MaxVoxel.X = FMath::Max(MaxVoxel.X, VoxelPosition.X);
		MaxVoxel.Y = FMath::Max(MaxVoxel.Y, VoxelPosition.Y);
		MaxVoxel.Z = FMath::Max(MaxVoxel.Z, VoxelPosition.Z);
	}

	int32 GridSizeX = static_cast<int32>((MaxVoxel.X - MinVoxel.X) / VoxelSize) + 1;
	int32 GridSizeY = static_cast<int32>((MaxVoxel.Y - MinVoxel.Y) / VoxelSize) + 1;
	int32 GridSizeZ = static_cast<int32>((MaxVoxel.Z - MinVoxel.Z) / VoxelSize) + 1;

	struct FVoxel
	{
		bool IsSolid = false;
		FLinearColor Color;
	};

	TArray<FVoxel> VoxelGrid;
	VoxelGrid.SetNum(GridSizeX * GridSizeY * GridSizeZ);

	auto GetVoxelIndex = [&](int32 X, int32 Y, int32 Z) -> int32 { return X + Y * GridSizeX + Z * GridSizeX * GridSizeY; };

	for (int32 i = 0; i < VoxelPositions.Num(); ++i)
	{
		FVector VoxelPosition = VoxelPositions[i];
		FLinearColor VoxelColor = VoxelColors[i];

		int32 X = static_cast<int32>((VoxelPosition.X - MinVoxel.X) / VoxelSize);
		int32 Y = static_cast<int32>((VoxelPosition.Y - MinVoxel.Y) / VoxelSize);
		int32 Z = static_cast<int32>((VoxelPosition.Z - MinVoxel.Z) / VoxelSize);

		if (X >= 0 && X < GridSizeX && Y >= 0 && Y < GridSizeY && Z >= 0 && Z < GridSizeZ)
		{
			int32 Index = GetVoxelIndex(X, Y, Z);
			FVoxel& Voxel = VoxelGrid[Index];
			Voxel.IsSolid = true;
			Voxel.Color = VoxelColor;
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

	for (int Axis = 0; Axis < 3; ++Axis)
	{
		int32 GridDims[3] = {GridSizeX, GridSizeY, GridSizeZ};
		int32 U = (Axis + 1) % 3;
		int32 V = (Axis + 2) % 3;

		int32 X[3] = {0, 0, 0};
		int32 Q[3] = {0, 0, 0};

		FVector Normal;
		if (Axis == 0)
		{
			Normal = FVector(1, 0, 0);
		}
		else if (Axis == 1)
		{
			Normal = FVector(0, 1, 0);
		}
		else
		{
			Normal = FVector(0, 0, 1);
		}

		struct FMaskVoxel
		{
			bool IsSolid = false;
			FLinearColor Color;
			bool IsFrontFace = false;
		};

		TArray<FMaskVoxel> Mask;
		Mask.SetNumZeroed(GridDims[U] * GridDims[V]);

		for (X[Axis] = -1; X[Axis] < GridDims[Axis];)
		{
			int32 n = 0;
			for (X[V] = 0; X[V] < GridDims[V]; ++X[V])
			{
				for (X[U] = 0; X[U] < GridDims[U]; ++X[U])
				{
					int32 CurrentX = X[0];
					int32 CurrentY = X[1];
					int32 CurrentZ = X[2];

					CurrentX = (Axis == 0) ? X[Axis] : X[0];
					CurrentY = (Axis == 1) ? X[Axis] : X[1];
					CurrentZ = (Axis == 2) ? X[Axis] : X[2];

					int32 AdjacentX = CurrentX;
					int32 AdjacentY = CurrentY;
					int32 AdjacentZ = CurrentZ;
					if (Axis == 0)
					{
						AdjacentX = X[Axis] + 1;
					}
					else if (Axis == 1)
					{
						AdjacentY = X[Axis] + 1;
					}
					else
					{
						AdjacentZ = X[Axis] + 1;
					}

					bool IsCurrentSolid = false;
					FLinearColor CurrentColor = FLinearColor::Black;

					if (CurrentX >= 0 && CurrentX < GridSizeX && CurrentY >= 0 && CurrentY < GridSizeY && CurrentZ >= 0 &&
						CurrentZ < GridSizeZ)
					{
						int32 CurrentIndex = GetVoxelIndex(CurrentX, CurrentY, CurrentZ);
						if (CurrentIndex >= 0 && CurrentIndex < VoxelGrid.Num())
						{
							const FVoxel& Voxel = VoxelGrid[CurrentIndex];
							IsCurrentSolid = Voxel.IsSolid;
							CurrentColor = Voxel.Color;
						}
					}

					bool IsAdjacentSolid = false;
					FLinearColor AdjacentColor = FLinearColor::Black;

					if (AdjacentX >= 0 && AdjacentX < GridSizeX && AdjacentY >= 0 && AdjacentY < GridSizeY && AdjacentZ >= 0 &&
						AdjacentZ < GridSizeZ)
					{
						int32 AdjacentIndex = GetVoxelIndex(AdjacentX, AdjacentY, AdjacentZ);
						if (AdjacentIndex >= 0 && AdjacentIndex < VoxelGrid.Num())
						{
							const FVoxel& Voxel = VoxelGrid[AdjacentIndex];
							IsAdjacentSolid = Voxel.IsSolid;
							AdjacentColor = Voxel.Color;
						}
					}

					FMaskVoxel& MaskVoxel = Mask[n++];

					if (IsCurrentSolid != IsAdjacentSolid)
					{
						MaskVoxel.IsSolid = true;
						MaskVoxel.Color = IsCurrentSolid ? CurrentColor : AdjacentColor;
						MaskVoxel.IsFrontFace = !IsCurrentSolid;
					}
					else
					{
						MaskVoxel.IsSolid = false;
					}
				}
			}

			++X[Axis];

			n = 0;
			for (int32 j = 0; j < GridDims[V]; ++j)
			{
				for (int32 i = 0; i < GridDims[U];)
				{
					if (Mask[n].IsSolid)
					{
						int32 w = 1;
						while (i + w < GridDims[U] && Mask[n + w].IsSolid && Mask[n + w].Color == Mask[n].Color &&
							   Mask[n + w].IsFrontFace == Mask[n].IsFrontFace)
						{
							++w;
						}

						int32 h = 1;
						bool Done = false;
						while (j + h < GridDims[V])
						{
							for (int32 k = 0; k < w; ++k)
							{
								int32 Index = n + k + h * GridDims[U];
								if (!Mask[Index].IsSolid || Mask[Index].Color != Mask[n].Color ||
									Mask[Index].IsFrontFace != Mask[n].IsFrontFace)
								{
									Done = true;
									break;
								}
							}
							if (Done)
							{
								break;
							}
							++h;
						}

						X[U] = i;
						X[V] = j;

						Q[U] = w;
						Q[V] = h;

						FVector Pos(0, 0, 0);
						Pos[Axis] = X[Axis] * VoxelSize;
						Pos[U] = X[U] * VoxelSize;
						Pos[V] = X[V] * VoxelSize;

						FVector Size(0, 0, 0);
						Size[Axis] = 0;
						Size[U] = Q[U] * VoxelSize;
						Size[V] = Q[V] * VoxelSize;

						FVector V0 = Pos;
						FVector V1 = Pos;
						V1[U] += Size[U];
						FVector V2 = V1;
						V2[V] += Size[V];
						FVector V3 = Pos;
						V3[V] += Size[V];

						V0 += MinVoxel;
						V1 += MinVoxel;
						V2 += MinVoxel;
						V3 += MinVoxel;

						if (Mask[n].IsFrontFace)
						{
							AddQuad(V0, V1, V2, V3, Mask[n].Color, Normal);
						}
						else
						{
							AddQuad(V0, V3, V2, V1, Mask[n].Color, -Normal);
						}

						for (int32 l = 0; l < h; ++l)
						{
							for (int32 k = 0; k < w; ++k)
							{
								Mask[n + k + l * GridDims[U]].IsSolid = false;
							}
						}

						i += w;
						n += w;
					}
					else
					{
						++i;
						++n;
					}
				}
			}
		}
	}

	FString MeshName = "GeneratedVoxelMesh";
	FString PackageName = "/Game/GeneratedMeshes/" + MeshName;
	UPackage* Package = CreatePackage(*PackageName);

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, FName(*MeshName), RF_Public | RF_Standalone);

	StaticMesh->SetLightingGuid();

	FMeshDescription MeshDescription;
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();
	TPolygonGroupAttributesRef<FName> MaterialSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

	TMap<int32, FVertexID> VertexIDMap;
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		const FVector& Vertex = Vertices[i];
		FVertexID VertexID = MeshDescription.CreateVertex();
		VertexPositions[VertexID] = static_cast<FVector3f>(Vertex);
		VertexIDMap.Add(i, VertexID);
	}

	FPolygonGroupID PolygonGroup = MeshDescription.CreatePolygonGroup();
	MaterialSlotNames[PolygonGroup] = FName("Default");

	for (int32 i = 0; i < Triangles.Num(); i += 3)
	{
		TArray<FVertexInstanceID> VertexInstanceIDs;
		for (int32 j = 0; j < 3; ++j)
		{
			int32 VertexIndex = Triangles[i + j];
			FVertexID VertexID = VertexIDMap[VertexIndex];
			FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(VertexID);

			const FLinearColor& VertexColor = VertexColors[VertexIndex];
			VertexInstanceColors[VertexInstanceID] = FVector4f(VertexColor);

			const FVector& Normal = Normals[VertexIndex];
			VertexInstanceNormals[VertexInstanceID] = (FVector3f) Normal;

			VertexInstanceIDs.Add(VertexInstanceID);
		}

		MeshDescription.CreatePolygon(PolygonGroup, VertexInstanceIDs);
	}

	StaticMesh->BuildFromMeshDescriptions({&MeshDescription});
	VoxelMeshComponent->SetStaticMesh(StaticMesh);

	if (VoxelMaterial)
	{
		VoxelMeshComponent->SetMaterial(0, VoxelMaterial);
	}

	SetupCollision(StaticMesh, MinVoxel, MaxVoxel);
}

void AVoxelObject::SetupCollision(UStaticMesh* StaticMesh, const FVector &MinVoxel, const FVector &MaxVoxel) const
{
	UBodySetup* BodySetup = StaticMesh->GetBodySetup();
	if (!BodySetup)
	{
		StaticMesh->CreateBodySetup();
	}

	BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	BodySetup->AggGeom.EmptyElements();

	FKBoxElem BoxElem;
	FVector Center = (MinVoxel + MaxVoxel) * 0.5f;
	FVector Extents = (MaxVoxel - MinVoxel) * 0.5f;

	BoxElem.Center = Center;
	BoxElem.X = Extents.X * 2.0f;
	BoxElem.Y = Extents.Y * 2.0f;
	BoxElem.Z = Extents.Z * 2.0f;

	BodySetup->AggGeom.BoxElems.Add(BoxElem);
	BodySetup->bHasCookedCollisionData = true;
	BodySetup->InvalidatePhysicsData();
	BodySetup->CreatePhysicsMeshes();

	VoxelMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VoxelMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}
