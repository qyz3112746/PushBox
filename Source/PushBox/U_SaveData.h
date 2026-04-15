// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SaveDataTypes.h"
#include "U_SaveData.generated.h"

UCLASS(BlueprintType)
class PUSHBOX_API U_SaveData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Data")
	FSaveData SaveData;
};
