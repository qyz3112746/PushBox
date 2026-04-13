// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PushBoxLevelRuntime.generated.h"

class ABoxActor;
class ABoxCell;
class ABoxTargetCell;
class AGridCellBase;
class UPushBoxLevelData;

DECLARE_MULTICAST_DELEGATE_OneParam(FPushBoxLevelFinishedDelegate, bool);

UCLASS(Blueprintable)
class APushBoxLevelRuntime : public AActor
{
	GENERATED_BODY()

public:
	APushBoxLevelRuntime();

	FPushBoxLevelFinishedDelegate OnLevelFinished;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Grid")
	float CellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Grid", meta = (ClampMin = "1.0"))
	float VisualBaseCellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Grid")
	FVector GridOrigin;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool LoadLevel(UPushBoxLevelData* InLevelData);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool TryMove(const FIntPoint& Direction);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool CheckVictory() const;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool ResetToInitialState();

	UFUNCTION(BlueprintPure, Category = "PushBox|Runtime")
	UPushBoxLevelData* GetCurrentLevelData() const { return CurrentLevelData; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Runtime")
	FName GetCurrentLevelId() const;

protected:
	virtual void BeginPlay() override;

private:
	struct FLevelValidationResult
	{
		bool bIsValid = false;
		FIntPoint PlayerSpawnCoord = FIntPoint::ZeroValue;
		TMap<TSubclassOf<ABoxTargetCell>, int32> TargetCountsByType;
	};

	UPROPERTY(Transient)
	UPushBoxLevelData* CurrentLevelData;

	TArray<TObjectPtr<AGridCellBase>> SpawnedCells;
	TArray<TObjectPtr<ABoxActor>> SpawnedBoxes;
	TMap<FIntPoint, TObjectPtr<ABoxActor>> BoxByCoord;
	TMap<FIntPoint, TObjectPtr<ABoxTargetCell>> TargetByCoord;
	TSet<FIntPoint> BlockingCoords;

	FIntPoint PlayerCoord;

	bool bHasAnnouncedVictory;

	bool ValidateLevelData(const UPushBoxLevelData* InLevelData, FLevelValidationResult& OutValidation) const;
	void ClearSpawnedLevel();
	AGridCellBase* SpawnCell(const TSubclassOf<AGridCellBase>& CellClass, const FIntPoint& GridCoord);
	void RegisterSpawnedCell(AGridCellBase* SpawnedCell);
	void UnregisterSpawnedCell(AGridCellBase* SpawnedCell);
	ABoxActor* SpawnBoxFromCell(ABoxCell* BoxCell);
	void RefreshTargetMatchedEffects();
	AGridCellBase* FindSpawnedCellAt(const FIntPoint& GridCoord) const;
	bool IsWithinGrid(const FIntPoint& GridCoord) const;
	FVector GridToWorld(const FIntPoint& GridCoord) const;
	void MovePlayerToCoord(const FIntPoint& GridCoord) const;
};
