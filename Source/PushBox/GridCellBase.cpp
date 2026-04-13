// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridCellBase.h"
#include "Components/StaticMeshComponent.h"
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
