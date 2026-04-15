// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PushBoxLevelData.generated.h"

class AGridCellBase;

USTRUCT(BlueprintType)
struct FPushBoxCellSpawnData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level")
	TSubclassOf<AGridCellBase> CellClass;
};

UCLASS(BlueprintType)
class PUSHBOX_API UPushBoxLevelData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level")
	FName LevelId = TEXT("DefaultLevel");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level", meta = (ClampMin = "1"))
	int32 GridWidth = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level", meta = (ClampMin = "1"))
	int32 GridHeight = 8;

	// Optional default cell class used to fill the whole grid before applying CellDefinitions overrides.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level")
	TSubclassOf<AGridCellBase> DefaultCellClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Level")
	TArray<FPushBoxCellSpawnData> CellDefinitions;
};
