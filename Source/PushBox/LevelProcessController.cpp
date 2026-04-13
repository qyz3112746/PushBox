// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelProcessController.h"

#include "PushBox.h"
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
}

UPushBoxLevelData* ALevelProcessController::GetInitialLevelData_Implementation()
{
	return DefaultLevelData;
}

UPushBoxLevelData* ALevelProcessController::DecideNextLevelData_Implementation(bool bLevelPassed, FName CurrentLevelId)
{
	return DefaultLevelData;
}

void ALevelProcessController::BeginPlay()
{
	Super::BeginPlay();

	if (!EnsureLevelRuntime())
	{
		return;
	}

	UPushBoxLevelData* InitialLevelData = GetInitialLevelData();
	if (!InitialLevelData)
	{
		UE_LOG(LogPushBox, Error, TEXT("LevelProcessController has no initial level data."));
		return;
	}

	LoadLevelData(InitialLevelData);
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
	return LevelRuntime->LoadLevel(LevelData);
}

bool ALevelProcessController::RestartCurrentLevel()
{
	if (!LevelRuntime)
	{
		return false;
	}

	SyncRuntimeOrigin();
	return LevelRuntime->ResetToInitialState();
}

void ALevelProcessController::HandleLevelFinished(bool bLevelPassed)
{
	if (!LevelRuntime)
	{
		return;
	}

	UPushBoxLevelData* NextLevelData = DecideNextLevelData(bLevelPassed, LevelRuntime->GetCurrentLevelId());
	if (NextLevelData)
	{
		LoadLevelData(NextLevelData);
	}
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
