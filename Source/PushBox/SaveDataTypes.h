// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveDataTypes.generated.h"

class AGridCellBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct PUSHBOX_API FCellDisplay
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelTool")
	TSubclassOf<AGridCellBase> CellType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelTool")
	FLinearColor BGColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelTool")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

USTRUCT(BlueprintType)
struct PUSHBOX_API FSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelTool")
	TArray<FCellDisplay> CellList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelTool", meta = (ClampMin = "2", ClampMax = "32"))
	int32 DisplaySize = 20;
};
