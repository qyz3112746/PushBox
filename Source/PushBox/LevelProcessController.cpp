// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelProcessController.h"

#include "PushBox.h"
#include "PushBoxFlowDirector.h"
#include "PushBoxLevelData.h"
#include "PushBoxLevelRuntime.h"
#include "Kismet/GameplayStatics.h"

ALevelProcessController::ALevelProcessController()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultLevelData = nullptr;
	LevelRuntimeClass = APushBoxLevelRuntime::StaticClass();
	CellSize = 100.0f;
	LevelOriginOffset = FVector::ZeroVector;
	LevelRuntime = nullptr;
	CurrentLevelIndex = INDEX_NONE;
	bPendingTransition = false;
	bPendingResultPassed = false;
	bAutoStartOnBeginPlay = true;
}

UPushBoxLevelData* ALevelProcessController::GetInitialLevelData_Implementation()
{
	return DefaultLevelData;
}

UPushBoxLevelData* ALevelProcessController::DecideNextLevelData_Implementation(bool bLevelPassed, FName CurrentLevelId)
{
	return DefaultLevelData;
}

ALevelProcessController* ALevelProcessController::ResolveNextProcessController_Implementation()
{
	return nullptr;
}

void ALevelProcessController::BeginPlay()
{
	Super::BeginPlay();

	if (!EnsureLevelRuntime())
	{
		return;
	}

	if (!bAutoStartOnBeginPlay)
	{
		return;
	}

	if (UGameplayStatics::GetActorOfClass(GetWorld(), APushBoxFlowDirector::StaticClass()))
	{
		return;
	}

	int32 InitialIndex = INDEX_NONE;
	UPushBoxLevelData* InitialLevelData = ResolveInitialLevelData(InitialIndex);
	if (!InitialLevelData)
	{
		UE_LOG(LogPushBox, Error, TEXT("LevelProcessController has no initial level data."));
		return;
	}

	if (InitialIndex != INDEX_NONE)
	{
		LoadLevelByIndex(InitialIndex);
	}
	else
	{
		LoadLevelData(InitialLevelData);
	}
}

bool ALevelProcessController::LoadLevelData(UPushBoxLevelData* LevelData)
{
	if (!EnsureLevelRuntime())
	{
		return false;
	}

	if (!LevelData)
	{
		UE_LOG(LogPushBox, Error, TEXT("LevelProcessController::LoadLevelData failed: LevelData is null."));
		return false;
	}

	SyncRuntimeOrigin();
	const bool bLoaded = LevelRuntime->LoadLevel(LevelData);
	if (bLoaded)
	{
		CurrentLevelIndex = FindIndexInSequence(LevelData);
		ClearPendingTransition();
	}
	return bLoaded;
}

bool ALevelProcessController::LoadLevelByIndex(int32 LevelIndex)
{
	if (!LevelSequence.IsValidIndex(LevelIndex))
	{
		UE_LOG(LogPushBox, Error, TEXT("LevelProcessController::LoadLevelByIndex failed: invalid index %d."), LevelIndex);
		return false;
	}

	UPushBoxLevelData* LevelData = LevelSequence[LevelIndex];
	if (!LevelData)
	{
		UE_LOG(LogPushBox, Error, TEXT("LevelProcessController::LoadLevelByIndex failed: LevelSequence[%d] is null."), LevelIndex);
		return false;
	}

	if (!LoadLevelData(LevelData))
	{
		return false;
	}

	CurrentLevelIndex = LevelIndex;
	return true;
}

bool ALevelProcessController::RestartCurrentLevel()
{
	if (!LevelRuntime)
	{
		return false;
	}

	SyncRuntimeOrigin();
	const bool bRestarted = LevelRuntime->ResetToInitialState();
	if (bRestarted)
	{
		ClearPendingTransition();
	}
	return bRestarted;
}

bool ALevelProcessController::ConfirmAdvanceToNextLevel()
{
	if (!LevelRuntime)
	{
		return false;
	}

	if (CurrentLevelIndex != INDEX_NONE)
	{
		const int32 NextIndex = CurrentLevelIndex + 1;
		if (LevelSequence.IsValidIndex(NextIndex))
		{
			return LoadLevelByIndex(NextIndex);
		}
	}

	ClearPendingTransition();
	OnFlowCompleted.Broadcast();
	return true;
}

bool ALevelProcessController::ConfirmRestartCurrentLevel()
{
	return RestartCurrentLevel();
}

void ALevelProcessController::CancelPendingTransition()
{
	ClearPendingTransition();
}

UPushBoxLevelData* ALevelProcessController::GetCurrentLevelData() const
{
	return LevelRuntime ? LevelRuntime->GetCurrentLevelData() : nullptr;
}

void ALevelProcessController::HandleLevelFinished(bool bLevelPassed)
{
	if (!LevelRuntime)
	{
		return;
	}

	bPendingTransition = true;
	bPendingResultPassed = bLevelPassed;
	OnPendingTransitionStarted.Broadcast(bLevelPassed, CurrentLevelIndex, LevelRuntime->GetCurrentLevelId());
}

bool ALevelProcessController::EnsureLevelRuntime()
{
	if (LevelRuntime)
	{
		return true;
	}

	LevelRuntime = Cast<APushBoxLevelRuntime>(UGameplayStatics::GetActorOfClass(GetWorld(), APushBoxLevelRuntime::StaticClass()));
	if (!LevelRuntime)
	{
		TSubclassOf<APushBoxLevelRuntime> SpawnClass = LevelRuntimeClass;
		if (!SpawnClass)
		{
			SpawnClass = APushBoxLevelRuntime::StaticClass();
		}
		LevelRuntime = GetWorld()->SpawnActor<APushBoxLevelRuntime>(SpawnClass, FVector::ZeroVector, FRotator::ZeroRotator);
	}

	if (LevelRuntime)
	{
		LevelRuntime->OnLevelFinished.RemoveAll(this);
		LevelRuntime->OnLevelFinished.AddUObject(this, &ALevelProcessController::HandleLevelFinished);
	}

	return LevelRuntime != nullptr;
}

void ALevelProcessController::SyncRuntimeOrigin() const
{
	if (!LevelRuntime)
	{
		return;
	}

	const FVector RuntimeOrigin = GetActorLocation() + LevelOriginOffset;
	LevelRuntime->CellSize = CellSize;
	LevelRuntime->GridOrigin = RuntimeOrigin;
	LevelRuntime->SetActorLocation(RuntimeOrigin);
}

UPushBoxLevelData* ALevelProcessController::ResolveInitialLevelData(int32& OutInitialIndex)
{
	OutInitialIndex = INDEX_NONE;

	if (LevelSequence.Num() > 0)
	{
		if (LevelSequence[0])
		{
			OutInitialIndex = 0;
			return LevelSequence[0];
		}
	}

	return GetInitialLevelData();
}

void ALevelProcessController::ClearPendingTransition()
{
	bPendingTransition = false;
	bPendingResultPassed = false;
}

int32 ALevelProcessController::FindIndexInSequence(const UPushBoxLevelData* LevelData) const
{
	if (!LevelData)
	{
		return INDEX_NONE;
	}

	return LevelSequence.IndexOfByPredicate([LevelData](const TObjectPtr<UPushBoxLevelData>& Item)
	{
		return Item.Get() == LevelData;
	});
}

void ALevelProcessController::ConfigureFlowLevelSequence(const TArray<UPushBoxLevelData*>& InLevelSequence)
{
	LevelSequence.Reset();
	LevelSequence.Reserve(InLevelSequence.Num());
	for (UPushBoxLevelData* LevelData : InLevelSequence)
	{
		LevelSequence.Add(LevelData);
	}
}
