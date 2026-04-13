// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoxActor.h"
#include "Components/StaticMeshComponent.h"

ABoxActor::ABoxActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	BoxMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMeshComponent->SetupAttachment(SceneRoot);
	BoxMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMeshComponent->SetVisibility(true);

	GridCoord = FIntPoint::ZeroValue;
}

void ABoxActor::SetGridCoord(const FIntPoint& InGridCoord)
{
	GridCoord = InGridCoord;
}

void ABoxActor::MoveToGridCoord(const FIntPoint& InGridCoord, const FVector& WorldLocation)
{
	GridCoord = InGridCoord;
	SetActorLocation(WorldLocation);
}

void ABoxActor::SetBoxMesh(UStaticMesh* InMesh)
{
	BoxMeshComponent->SetStaticMesh(InMesh);
}
