// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelPreviewSubsystem.h"

#include "Editor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "GridCellBase.h"
#include "LevelProcessController.h"
#include "PushBoxLevelData.h"

bool UPushBoxLevelPreviewSubsystem::StartPreview(ALevelProcessController* InController, UPushBoxLevelData* InLevelData, int32 InSequenceIndex)
{
	if (!InController || !InLevelData)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartPreview failed: invalid controller or level data."));
		return false;
	}

	UWorld* World = InController->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartPreview failed: controller world is null."));
		return false;
	}

	ClearPreview();

	const int32 GridWidth = FMath::Max(1, InLevelData->GridWidth);
	const int32 GridHeight = FMath::Max(1, InLevelData->GridHeight);
	const FVector GridOrigin = InController->GetConfiguredGridOrigin();
	const float CellSize = InController->GetConfiguredCellSize();
	const float VisualBaseCellSize = InController->GetConfiguredVisualBaseCellSize();
	const TSubclassOf<APawn> PreviewPawnClass = ResolvePreviewPawnClass(World);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FIntPoint GridCoord(X, Y);
			TSubclassOf<AGridCellBase> CellClass = InLevelData->GetCellAt(GridCoord);
			if (!CellClass)
			{
				CellClass = InLevelData->DefaultCellClass;
			}

			if (!CellClass)
			{
				continue;
			}

			const FVector SpawnLocation = GridOrigin + FVector(X * CellSize, Y * CellSize, 0.0f);
			const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;

			AGridCellBase* SpawnedCell = World->SpawnActorDeferred<AGridCellBase>(
				CellClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);
			if (!SpawnedCell)
			{
				continue;
			}

			const float SafeBaseSize = FMath::Max(VisualBaseCellSize, 1.0f);
			const float UniformScale = CellSize / SafeBaseSize;
			SpawnedCell->SetActorScale3D(FVector(UniformScale));
			SpawnedCell->SetGridCoord(GridCoord);
			SpawnedCell->bIsEditorOnlyActor = true;
			SpawnedCell->SetActorLabel(FString::Printf(TEXT("Preview_Cell_%d_%d"), X, Y));

			SpawnedCell->FinishSpawning(SpawnTransform);
			if (InController->GetRootComponent())
			{
				SpawnedCell->AttachToComponent(InController->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			}
			SpawnedPreviewActors.Add(SpawnedCell);

			FCellEditorPreviewContext PreviewContext;
			PreviewContext.GridOrigin = GridOrigin;
			PreviewContext.CellSize = CellSize;
			PreviewContext.VisualBaseCellSize = VisualBaseCellSize;
			PreviewContext.GridCoord = GridCoord;
			PreviewContext.LevelData = InLevelData;
			PreviewContext.PreviewPawnClass = PreviewPawnClass;
			PreviewContext.PreviewAttachActor = InController;
			SpawnedCell->BuildEditorPreview(PreviewContext);

			for (AActor* ExtraActor : PreviewContext.SpawnedActors)
			{
				if (ExtraActor)
				{
					SpawnedPreviewActors.Add(ExtraActor);
				}
			}
		}
	}

	ActiveProcessController = InController;
	ActiveLevelData = InLevelData;

	UE_LOG(LogTemp, Log, TEXT("Preview started: Controller=%s, Level=%s, Index=%d, SpawnedActors=%d"),
		*GetNameSafe(InController),
		*GetNameSafe(InLevelData),
		InSequenceIndex,
		SpawnedPreviewActors.Num());

	return true;
}

void UPushBoxLevelPreviewSubsystem::ClearPreview()
{
	if (SpawnedPreviewActors.Num() == 0)
	{
		ActiveProcessController = nullptr;
		ActiveLevelData = nullptr;
		return;
	}

	for (int32 Index = SpawnedPreviewActors.Num() - 1; Index >= 0; --Index)
	{
		AActor* Actor = SpawnedPreviewActors[Index].Get();
		if (!Actor)
		{
			continue;
		}

		Actor->Destroy();
	}

	SpawnedPreviewActors.Reset();
	ActiveProcessController = nullptr;
	ActiveLevelData = nullptr;
}

TSubclassOf<APawn> UPushBoxLevelPreviewSubsystem::ResolvePreviewPawnClass(UWorld* InWorld) const
{
	if (!InWorld)
	{
		return nullptr;
	}

	if (AGameModeBase* GameMode = InWorld->GetAuthGameMode())
	{
		return GameMode->DefaultPawnClass;
	}

	const AWorldSettings* WorldSettings = InWorld->GetWorldSettings();
	if (WorldSettings && WorldSettings->DefaultGameMode)
	{
		const AGameModeBase* GameModeCDO = WorldSettings->DefaultGameMode->GetDefaultObject<AGameModeBase>();
		return GameModeCDO ? GameModeCDO->DefaultPawnClass : nullptr;
	}

	return nullptr;
}
