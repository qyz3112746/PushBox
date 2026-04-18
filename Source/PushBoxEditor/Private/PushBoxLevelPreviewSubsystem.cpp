// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelPreviewSubsystem.h"

#include "Editor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "GridCellBase.h"
#include "LevelProcessController.h"
#include "PushBoxLevelData.h"
#include "EngineUtils.h"

namespace
{
	static const FName PushBoxPreviewActorTag(TEXT("PushBoxPreviewActor"));
}

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
			RegisterPreviewActor(SpawnedCell);

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
				RegisterPreviewActor(ExtraActor);
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
	TArray<AActor*> ActorsToDestroy;
	for (const TObjectPtr<AActor>& WeakActor : SpawnedPreviewActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			ActorsToDestroy.AddUnique(Actor);
		}
	}

	// Recovery path for editor restart: subsystem cache is empty, but preview actors may still exist.
	UWorld* ScanWorld = nullptr;
	if (ActiveProcessController)
	{
		ScanWorld = ActiveProcessController->GetWorld();
	}
	if (!ScanWorld && GEditor)
	{
		ScanWorld = GEditor->GetEditorWorldContext().World();
	}
	if (ScanWorld)
	{
		GatherTaggedPreviewActors(ScanWorld, ActorsToDestroy);
	}

	for (int32 Index = ActorsToDestroy.Num() - 1; Index >= 0; --Index)
	{
		AActor* Actor = ActorsToDestroy[Index];
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

void UPushBoxLevelPreviewSubsystem::RegisterPreviewActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	InActor->bIsEditorOnlyActor = true;
	InActor->SetFlags(RF_Transient);
	InActor->Tags.AddUnique(PushBoxPreviewActorTag);
	SpawnedPreviewActors.AddUnique(InActor);
}

void UPushBoxLevelPreviewSubsystem::GatherTaggedPreviewActors(UWorld* InWorld, TArray<AActor*>& OutActors) const
{
	if (!InWorld)
	{
		return;
	}

	for (TActorIterator<AActor> It(InWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->Tags.Contains(PushBoxPreviewActorTag))
		{
			OutActors.AddUnique(Actor);
		}
	}
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
