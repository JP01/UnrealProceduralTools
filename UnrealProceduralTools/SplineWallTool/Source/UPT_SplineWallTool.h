#pragma once

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GC_SplineTool.generated.h"

USTRUCT()
struct FSplineDoorParams
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere)
	UStaticMesh* DoorMesh;

	UPROPERTY(EditAnywhere)
	uint32 DoorPosition{ 0 };

	UPROPERTY(EditAnywhere)
	bool bFlip{ false };
};

UCLASS()
class GOBLINCOP_API AUPT_SplineWallTool : public AActor
{
	GENERATED_BODY()

public:
	AUPT_SplineWallTool();

	UPROPERTY(EditAnywhere, Category = "Walls")
	UStaticMesh* WallMesh;

	UPROPERTY(EditAnywhere, Category = "Walls")
	FVector WallRotationOffset{ 0.0, 90.0, 0.0 };

	UPROPERTY(EditAnywhere, Category = "Corners")
	UStaticMesh* CornerMesh;

	UPROPERTY(EditAnywhere, Category = "Corners")
	bool bUseCornerMesh{ true };

	UPROPERTY(EditAnywhere, Category = "Corners")
	bool bPlaceCornerMeshAtEnd{ false };

	UPROPERTY(EditAnywhere, Category = "Doors")
	TArray<FSplineDoorParams> Doors;

protected:
	UPROPERTY(EditDefaultsOnly)
	USplineComponent* GuideSpline;

	UPROPERTY()
	UInstancedStaticMeshComponent* InstancedWallMesh;

	UPROPERTY()
	UInstancedStaticMeshComponent* InstancedCornerMesh;

	TArray<UStaticMeshComponent> DoorMeshes;

public:
	virtual void OnConstruction(const FTransform& Transform) override;
};
