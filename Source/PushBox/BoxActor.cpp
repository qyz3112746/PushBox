// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoxActor.h"
#include "Components/StaticMeshComponent.h"

ABoxActor::ABoxActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	BoxMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMeshComponent->SetupAttachment(SceneRoot);
	BoxMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMeshComponent->SetVisibility(true);
	BoxMeshComponent->SetReceivesDecals(false);

	GridCoord = FIntPoint::ZeroValue;
	TargetCoord = FIntPoint::ZeroValue;
	bIsInterpolatingMove = false;
	MoveStartLocation = FVector::ZeroVector;
	MoveTargetLocation = FVector::ZeroVector;
	MoveStartTime = 0.0f;
	MoveDuration = 0.0f;
}

void ABoxActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsInterpolatingMove)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || MoveDuration <= KINDA_SMALL_NUMBER)
	{
		SetActorLocation(MoveTargetLocation);
		bIsInterpolatingMove = false;
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - MoveStartTime;
	const float Alpha = FMath::Clamp(Elapsed / MoveDuration, 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(MoveStartLocation, MoveTargetLocation, Alpha));
	if (Alpha >= 1.0f)
	{
		bIsInterpolatingMove = false;
	}
}

void ABoxActor::SetGridCoord(const FIntPoint& InGridCoord)
{
	GridCoord = InGridCoord;
}

void ABoxActor::MoveToGridCoord(const FIntPoint& InGridCoord, const FVector& WorldLocation, float InMoveDuration)
{
	GridCoord = InGridCoord;
	if (InMoveDuration <= KINDA_SMALL_NUMBER || !GetWorld())
	{
		SetActorLocation(WorldLocation);
		bIsInterpolatingMove = false;
		return;
	}

	MoveStartLocation = GetActorLocation();
	MoveTargetLocation = WorldLocation;
	MoveStartTime = GetWorld()->GetTimeSeconds();
	MoveDuration = InMoveDuration;
	bIsInterpolatingMove = true;
}

void ABoxActor::SetBoxMesh(UStaticMesh* InMesh)
{
	BoxMeshComponent->SetStaticMesh(InMesh);
}

void ABoxActor::SetTargetCoord(const FIntPoint& InTargetCoord)
{
	TargetCoord = InTargetCoord;
}
