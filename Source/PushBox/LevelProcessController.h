// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelProcessController.generated.h"

class UPushBoxLevelData;
class APushBoxLevelRuntime;

UCLASS(Blueprintable)
class ALevelProcessController : public AActor
{
	GENERATED_BODY()

public:
	ALevelProcessController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	UPushBoxLevelData* DefaultLevelData;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PushBox|Flow")
	UPushBoxLevelData* GetInitialLevelData();
	virtual UPushBoxLevelData* GetInitialLevelData_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PushBox|Flow")
	UPushBoxLevelData* DecideNextLevelData(bool bLevelPassed, FName CurrentLevelId);
	virtual UPushBoxLevelData* DecideNextLevelData_Implementation(bool bLevelPassed, FName CurrentLevelId);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool LoadLevelData(UPushBoxLevelData* LevelData);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool RestartCurrentLevel();

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	APushBoxLevelRuntime* GetLevelRuntime() const { return LevelRuntime; }

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

private:
	UFUNCTION()
	void HandleLevelFinished(bool bLevelPassed);

	bool EnsureLevelRuntime();
	void SyncRuntimeOrigin() const;
};
