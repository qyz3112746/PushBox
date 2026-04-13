// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridCellBase.h"
#include "BoxActor.h"
#include "PushBoxLevelRuntime.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

AGridCellBase::AGridCellBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	CellMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CellMesh"));
	CellMeshComponent->SetupAttachment(SceneRoot);
	CellMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CellNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CellNiagara"));
	CellNiagaraComponent->SetupAttachment(SceneRoot);
	CellNiagaraComponent->bAutoActivate = true;

	bBlocksMovement = false;
	GridCoord = FIntPoint::ZeroValue;
	CachedRuntime = nullptr;
}

void AGridCellBase::SetGridCoord(const FIntPoint& InGridCoord)
{
	GridCoord = InGridCoord;
}

void AGridCellBase::SetBlocksMovement(bool bInBlocksMovement)
{
	bBlocksMovement = bInBlocksMovement;
}

UStaticMesh* AGridCellBase::GetCellStaticMesh() const
{
	return CellMeshComponent ? CellMeshComponent->GetStaticMesh() : nullptr;
}

FTransform AGridCellBase::GetCellMeshRelativeTransform() const
{
	return CellMeshComponent ? CellMeshComponent->GetRelativeTransform() : FTransform::Identity;
}

bool AGridCellBase::CanPlayerEnter_Implementation(const FCellMoveContext& Context) const
{
	return !bBlocksMovement;
}

bool AGridCellBase::CanPlayerExit_Implementation(const FCellMoveContext& Context) const
{
	return true;
}

void AGridCellBase::OnPlayerEnter_Implementation(const FCellMoveContext& Context)
{
}

void AGridCellBase::OnPlayerExit_Implementation(const FCellMoveContext& Context)
{
}

bool AGridCellBase::CanBoxEnter_Implementation(const FCellMoveContext& Context) const
{
	return !bBlocksMovement;
}

bool AGridCellBase::CanBoxExit_Implementation(const FCellMoveContext& Context) const
{
	return true;
}

void AGridCellBase::OnBoxEnter_Implementation(const FCellMoveContext& Context)
{
}

void AGridCellBase::OnBoxExit_Implementation(const FCellMoveContext& Context)
{
}

ABoxActor* AGridCellBase::SpawnBoxAt(TSubclassOf<ABoxActor> BoxClass, const FIntPoint& TargetCoord)
{
	if (!CachedRuntime)
	{
		CachedRuntime = GetWorld() ? Cast<APushBoxLevelRuntime>(UGameplayStatics::GetActorOfClass(GetWorld(), APushBoxLevelRuntime::StaticClass())) : nullptr;
	}

	if (!CachedRuntime)
	{
		return nullptr;
	}

	return CachedRuntime->RegisterSpawnedBox(GridCoord, BoxClass, TargetCoord);
}
