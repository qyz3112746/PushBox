// Copyright Epic Games, Inc. All Rights Reserved.

#include "GridCellBase.h"
#include "BoxActor.h"
#include "PushBoxLevelRuntime.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
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

void AGridCellBase::BuildEditorPreview_Implementation(FCellEditorPreviewContext& PreviewContext)
{
}

ABoxActor* AGridCellBase::SpawnPreviewBoxAt(FCellEditorPreviewContext& PreviewContext, TSubclassOf<ABoxActor> BoxClass, const FIntPoint& TargetCoord)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TSubclassOf<ABoxActor> SpawnClass = BoxClass;
	if (!SpawnClass)
	{
		SpawnClass = ABoxActor::StaticClass();
	}

	const FVector WorldLocation = PreviewContext.GridOrigin + FVector(
		PreviewContext.GridCoord.X * PreviewContext.CellSize,
		PreviewContext.GridCoord.Y * PreviewContext.CellSize,
		0.0f
	);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	ABoxActor* SpawnedBox = World->SpawnActor<ABoxActor>(SpawnClass, WorldLocation, FRotator::ZeroRotator, SpawnParams);
	if (!SpawnedBox)
	{
		return nullptr;
	}

	const float SafeBaseSize = FMath::Max(PreviewContext.VisualBaseCellSize, 1.0f);
	const float UniformScale = PreviewContext.CellSize / SafeBaseSize;
	SpawnedBox->SetActorScale3D(FVector(UniformScale));
	SpawnedBox->SetGridCoord(PreviewContext.GridCoord);
	SpawnedBox->SetTargetCoord(TargetCoord);
	SpawnedBox->bIsEditorOnlyActor = true;
	if (PreviewContext.PreviewAttachActor && PreviewContext.PreviewAttachActor->GetRootComponent())
	{
		SpawnedBox->AttachToComponent(
			PreviewContext.PreviewAttachActor->GetRootComponent(),
			FAttachmentTransformRules::KeepWorldTransform
		);
	}
	PreviewContext.SpawnedActors.Add(SpawnedBox);
	return SpawnedBox;
}

APawn* AGridCellBase::SpawnPreviewPawn(FCellEditorPreviewContext& PreviewContext, TSubclassOf<APawn> PawnClass)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (!PawnClass)
	{
		return nullptr;
	}

	const FVector WorldLocation = PreviewContext.GridOrigin + FVector(
		PreviewContext.GridCoord.X * PreviewContext.CellSize,
		PreviewContext.GridCoord.Y * PreviewContext.CellSize,
		0.0f
	);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	APawn* SpawnedPawn = World->SpawnActor<APawn>(PawnClass, WorldLocation, FRotator::ZeroRotator, SpawnParams);
	if (!SpawnedPawn)
	{
		return nullptr;
	}

	SpawnedPawn->bIsEditorOnlyActor = true;
	if (PreviewContext.PreviewAttachActor && PreviewContext.PreviewAttachActor->GetRootComponent())
	{
		SpawnedPawn->AttachToComponent(
			PreviewContext.PreviewAttachActor->GetRootComponent(),
			FAttachmentTransformRules::KeepWorldTransform
		);
	}
	PreviewContext.SpawnedActors.Add(SpawnedPawn);
	return SpawnedPawn;
}
