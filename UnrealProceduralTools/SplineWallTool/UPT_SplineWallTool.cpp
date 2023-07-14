#include "SplineWallTool/UPT_SplineWallTool.h"

#include <Kismet/KismetMathLibrary.h>

// Sets default values
AUPT_SplineWallTool::AUPT_SplineWallTool()
{
	PrimaryActorTick.bCanEverTick = false;

	GuideSpline = CreateDefaultSubobject<USplineComponent>(TEXT("GuideSpline"));
	SetRootComponent(GuideSpline);
	InstancedWallMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedWallMesh"));
	InstancedCornerMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedCornerMesh"));
}

void AUPT_SplineWallTool::OnConstruction(const FTransform& Transform)
{
	if (!WallMesh || !CornerMesh)
		return;

	// Refresh to ensure we don't duplicate anything
	InstancedWallMesh->ClearInstances();
	InstancedCornerMesh->ClearInstances();

	// Setup the static meshes
	InstancedWallMesh->SetStaticMesh(WallMesh);
	InstancedCornerMesh->SetStaticMesh(CornerMesh);

	// Figure out wall width so we know how many to place between spline points
	float WallWidth = WallMesh->GetBounds().BoxExtent.Y * 2.0; // The extent is a half width, so we multiply by 2

	// Place walls

	// Start at index 1 so we can check backwards
	int LastIndex = GuideSpline->GetNumberOfSplinePoints() - 1;
	for (int i = 1; i <= LastIndex; i++)
	{
		// Use these to find the number of walls to place between the points
		FVector PointA = GuideSpline->GetLocationAtSplinePoint(i - 1, ESplineCoordinateSpace::World);
		FVector PointB = GuideSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);

		int NumberOfWallsThisSection = UKismetMathLibrary::FFloor(FVector::Distance(PointA, PointB) / WallWidth);

		// Find the line that the spline traces. IE the line between the current and previous locations.
		FVector LineBA = UKismetMathLibrary::Normal(PointB - PointA);

		for (int j = 0; j < NumberOfWallsThisSection; j++)
		{
			// Get the location to place the wall section instance
			FVector Location = PointA + (j * WallWidth * LineBA);

			// Rotate by 90 deg as this rotation is hardcoded into the wall meshes we are using
			FRotator Rotation = UKismetMathLibrary::FindLookAtRotation(PointA, PointB);
			Rotation = UKismetMathLibrary::ComposeRotators(Rotation, WallRotationOffset.Rotation());

			// Create the wall section instance
			InstancedWallMesh->AddInstance(
				UKismetMathLibrary::MakeTransform(Location, Rotation, FVector(1.0, 1.0, 1.0)));
		}
	}

	// Place Corners
	if (bUseCornerMesh)
	{
		int CornersLastIndex = bPlaceCornerMeshAtEnd ? GuideSpline->GetNumberOfSplinePoints() - 1
													 : GuideSpline->GetNumberOfSplinePoints() - 2;

		for (int i = 0; i <= CornersLastIndex; i++)
		{
			FVector Location = GuideSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			InstancedCornerMesh->AddInstance(FTransform(Location));
		}
	}

	// Place Doors
	TArray<uint32> WallInstancesToRemove;
	for (int i = 0; i < Doors.Num(); i++)
	{
		auto Door = Doors[i];
		if (!InstancedWallMesh->IsValidInstance(Door.DoorPosition))
			break;
		if (!Door.DoorMesh)
			break;

		FTransform DoorInstanceTransform;
		InstancedWallMesh->GetInstanceTransform(Door.DoorPosition, DoorInstanceTransform);

		// Create a unique door mesh. We don't use instances here since we want to allow different door meshes.
		UStaticMeshComponent* DoorMeshComponent = NewObject<UStaticMeshComponent>(
			this, UStaticMeshComponent::StaticClass(), *FString::Printf(TEXT("DOOR_%i"), i));
		DoorMeshComponent->RegisterComponent();
		DoorMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		DoorMeshComponent->SetStaticMesh(Door.DoorMesh);
		if (Door.bFlip)
		{
			FVector Scale = DoorInstanceTransform.GetScale3D();
			Scale.X = -Scale.X;
			DoorInstanceTransform.SetScale3D(Scale);
		}
		DoorMeshComponent->SetWorldTransform(DoorInstanceTransform);

		// We can now use the wall width to find the number of walls per door
		int NumberOfWallsPerDoor = UKismetMathLibrary::FCeil(Door.DoorMesh->GetBounds().BoxExtent.Y * 2.0 / WallWidth);
		for (int j = 0; j < NumberOfWallsPerDoor; j++)
		{
			int SplineIndex = j + Door.DoorPosition;
			if (InstancedWallMesh->IsValidInstance(SplineIndex))
			{
				WallInstancesToRemove.Add(SplineIndex);
			}
		}
	}

	// Remove the wall sections that overlap with the door.
	for (auto& Index : WallInstancesToRemove)
	{
		// If we remove the instance entirely it will mess up the index / IDs,
		// so instead we can just scale each instance we want to remove to 0!
		InstancedWallMesh->UpdateInstanceTransform(
			Index, FTransform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(0, 0, 0)));
	}
}
