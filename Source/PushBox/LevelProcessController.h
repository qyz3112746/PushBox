// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelProcessController.generated.h"

class UPushBoxLevelData;
class APushBoxLevelRuntime;
class ALevelProcessController;

USTRUCT(BlueprintType)
struct FPushBoxLevelSequenceEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Flow")
	TObjectPtr<UPushBoxLevelData> LevelData = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPushBoxPendingTransitionSignature, bool, bLevelPassed, int32, CurrentIndex, FName, CurrentLevelId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPushBoxFlowCompletedSignature);

UCLASS(Blueprintable)
class PUSHBOX_API ALevelProcessController : public AActor
{
	GENERATED_BODY()

public:
	ALevelProcessController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	UPushBoxLevelData* DefaultLevelData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	TArray<FPushBoxLevelSequenceEntry> LevelSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	bool bAutoStartOnBeginPlay;

	UPROPERTY(BlueprintAssignable, Category = "PushBox|Flow")
	FPushBoxPendingTransitionSignature OnPendingTransitionStarted;

	UPROPERTY(BlueprintAssignable, Category = "PushBox|Flow")
	FPushBoxFlowCompletedSignature OnFlowCompleted;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PushBox|Flow")
	UPushBoxLevelData* GetInitialLevelData();
	virtual UPushBoxLevelData* GetInitialLevelData_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PushBox|Flow", meta = (DeprecatedFunction, DeprecationMessage = "Use LevelSequence and ConfirmAdvanceToNextLevel flow."))
	UPushBoxLevelData* DecideNextLevelData(bool bLevelPassed, FName CurrentLevelId);
	virtual UPushBoxLevelData* DecideNextLevelData_Implementation(bool bLevelPassed, FName CurrentLevelId);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool LoadLevelData(UPushBoxLevelData* LevelData);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool LoadLevelByIndex(int32 LevelIndex);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool RestartCurrentLevel();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool ConfirmAdvanceToNextLevel();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool ConfirmRestartCurrentLevel();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	void CancelPendingTransition();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PushBox|Flow")
	ALevelProcessController* ResolveNextProcessController();
	virtual ALevelProcessController* ResolveNextProcessController_Implementation();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	void ConfigureFlowLevelSequence(const TArray<UPushBoxLevelData*>& InLevelSequence);

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	int32 GetCurrentLevelIndex() const { return CurrentLevelIndex; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	UPushBoxLevelData* GetCurrentLevelData() const;

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	bool IsPendingTransition() const { return bPendingTransition; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	APushBoxLevelRuntime* GetLevelRuntime() const { return LevelRuntime; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	float GetConfiguredCellSize() const { return CellSize; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	FVector GetConfiguredGridOrigin() const { return GetActorLocation() + LevelOriginOffset; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	float GetConfiguredVisualBaseCellSize() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	TSubclassOf<APushBoxLevelRuntime> LevelRuntimeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow", meta = (ClampMin = "1.0"))
	float CellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	FVector LevelOriginOffset;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "PushBox|Flow")
	TObjectPtr<APushBoxLevelRuntime> LevelRuntime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "PushBox|Flow")
	int32 CurrentLevelIndex;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "PushBox|Flow")
	bool bPendingTransition;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "PushBox|Flow")
	bool bPendingResultPassed;

private:
	UFUNCTION()
	void HandleLevelFinished(bool bLevelPassed);

	UPushBoxLevelData* ResolveInitialLevelData(int32& OutInitialIndex);
	void ClearPendingTransition();
	int32 FindIndexInSequence(const UPushBoxLevelData* LevelData) const;

	bool EnsureLevelRuntime();
	void SyncRuntimeOrigin() const;
	void BindRuntimeFinishedCallback();
};
