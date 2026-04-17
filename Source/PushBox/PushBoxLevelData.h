// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PushBoxLevelData.generated.h"

class AGridCellBase;

USTRUCT(BlueprintType)
struct FPushBoxLevelGridRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level")
	TArray<TSubclassOf<AGridCellBase>> Cells;
};

UCLASS(BlueprintType)
class PUSHBOX_API UPushBoxLevelData : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PushBox|Level")
	void CopyFromLevelData(const UPushBoxLevelData* SourceLevelData);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Level")
	void InitializeGrid(int32 InWidth, int32 InHeight, TSubclassOf<AGridCellBase> FillCellClass);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Level")
	void ResizeGrid(int32 NewWidth, int32 NewHeight, TSubclassOf<AGridCellBase> FillCellClass);

	UFUNCTION(BlueprintPure, Category = "PushBox|Level")
	TSubclassOf<AGridCellBase> GetCellAt(FIntPoint GridCoord) const;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Level")
	bool SetCellAt(FIntPoint GridCoord, TSubclassOf<AGridCellBase> CellClass);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Level")
	void NormalizeGrid(TSubclassOf<AGridCellBase> FillCellClass);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Level")
	FName LevelId = TEXT("DefaultLevel");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Level", meta = (ClampMin = "1"))
	int32 GridWidth = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Level", meta = (ClampMin = "1"))
	int32 GridHeight = 8;

	// Optional default cell class used to fill empty slots in GridRows.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Level")
	TSubclassOf<AGridCellBase> DefaultCellClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Level")
	TArray<FPushBoxLevelGridRow> GridRows;
};
