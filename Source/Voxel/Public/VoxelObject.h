#pragma once

#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
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
	void CreateGreedyVoxelMesh(const TArray<FVector>& VoxelPositions, const TArray<FLinearColor>& VoxelColors) const;
	void SetupCollision(UStaticMesh* StaticMesh, const FVector &MinVoxel, const FVector &MaxVoxel) const;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* VoxelMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (AllowPrivateAccess = "true"))
	float VoxelSize = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (AllowPrivateAccess = "true", FilePathFilter = "vox"))
	FFilePath ModelFilePath;

	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	UMaterialInterface* VoxelMaterial;

	FVoxelColorPalette ColorPalette;
};
