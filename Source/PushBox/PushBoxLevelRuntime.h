// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PushBoxLevelRuntime.generated.h"

class ABoxActor;
class ABoxCell;
class ABoxTargetCell;
class AGridCellBase;
class ALevelProcessController;
class UPushBoxLevelData;
enum class ECellMoverType : uint8;
struct FCellMoveContext;

DECLARE_MULTICAST_DELEGATE_OneParam(FPushBoxLevelFinishedDelegate, bool);

UCLASS(Blueprintable)
class APushBoxLevelRuntime : public AActor
{
	GENERATED_BODY()

public:
	APushBoxLevelRuntime();
	virtual void Tick(float DeltaSeconds) override;

	FPushBoxLevelFinishedDelegate OnLevelFinished;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Grid")
	float CellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Grid", meta = (ClampMin = "1.0"))
	float VisualBaseCellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Grid")
	FVector GridOrigin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Movement", meta = (ClampMin = "0.0"))
	float InputInterval;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Movement", meta = (ClampMin = "0.0"))
	float MoveDuration;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool LoadLevel(UPushBoxLevelData* InLevelData);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool TryMove(const FIntPoint& Direction);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool CheckVictory() const;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	bool ResetToInitialState();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	ABoxActor* RegisterSpawnedBox(const FIntPoint& SpawnCoord, TSubclassOf<ABoxActor> BoxClass, const FIntPoint& TargetCoord);

	UFUNCTION(BlueprintPure, Category = "PushBox|Runtime")
	UPushBoxLevelData* GetCurrentLevelData() const { return CurrentLevelData; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Runtime")
	FName GetCurrentLevelId() const;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Runtime")
	void SetActiveProcessController(ALevelProcessController* InController);

	UFUNCTION(BlueprintPure, Category = "PushBox|Runtime")
	ALevelProcessController* GetActiveProcessController() const { return ActiveProcessController.Get(); }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	UPushBoxLevelData* CurrentLevelData;

	TWeakObjectPtr<ALevelProcessController> ActiveProcessController;

	TArray<TObjectPtr<AGridCellBase>> SpawnedCells;
	TMap<FIntPoint, TObjectPtr<ABoxActor>> BoxByCoord;
	TMap<TObjectPtr<ABoxActor>, FIntPoint> BoxCoordByActor;
	TSet<TObjectPtr<ABoxActor>> LiveBoxes;
	TMap<FIntPoint, TObjectPtr<ABoxTargetCell>> TargetByCoord;
	TSet<FIntPoint> BlockingCoords;

	FIntPoint PlayerCoord;

	bool bHasAnnouncedVictory;
	double NextMoveAllowedTime;
	bool bIsPlayerInterpolatingMove;
	FVector PlayerMoveStartLocation;
	FVector PlayerMoveTargetLocation;
	float PlayerMoveStartTime;
	float PlayerMoveDuration;
	bool bHasPlayerLockedZ;
	float PlayerLockedZ;

	void ClearSpawnedLevel();
	AGridCellBase* SpawnCell(const TSubclassOf<AGridCellBase>& CellClass, const FIntPoint& GridCoord);
	void RegisterSpawnedCell(AGridCellBase* SpawnedCell);
	void UnregisterSpawnedCell(AGridCellBase* SpawnedCell);
	void RefreshTargetMatchedEffects();
	AGridCellBase* FindSpawnedCellAt(const FIntPoint& GridCoord) const;
	TArray<AGridCellBase*> FindAllCellsAt(const FIntPoint& GridCoord) const;
	bool CanMoverExitCell(ECellMoverType MoverType, const FCellMoveContext& Context) const;
	bool CanMoverEnterCell(ECellMoverType MoverType, const FCellMoveContext& Context) const;
	void NotifyMoverExitCell(ECellMoverType MoverType, const FCellMoveContext& Context);
	void NotifyMoverEnterCell(ECellMoverType MoverType, const FCellMoveContext& Context);
	bool IsWithinGrid(const FIntPoint& GridCoord) const;
	FVector GridToWorld(const FIntPoint& GridCoord) const;
	void MovePlayerToCoord(const FIntPoint& GridCoord, bool bInstant);
};
