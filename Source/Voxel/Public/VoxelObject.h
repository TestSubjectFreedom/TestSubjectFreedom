#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "VoxelColorPalette.h"

#include "VoxelObject.generated.h"

UCLASS()
class VOXEL_API AVoxelObject : public AActor
{
	GENERATED_BODY()

public:
	AVoxelObject();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Voxel")
	void LoadModelInEditor();
#endif

private:
	void LoadVoxelFromFile();
	void CreateGreedyVoxelMesh(const TArray<FVector>& VoxelPositions, const TArray<FLinearColor>& VoxelColors);

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* VoxelMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (AllowPrivateAccess = "true"))
	float VoxelSize = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (AllowPrivateAccess = "true", FilePathFilter = "vox"))
	FFilePath ModelFilePath;

	FVoxelColorPalette ColorPalette;
};
