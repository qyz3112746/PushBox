// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PushBoxFlowDataAsset.generated.h"

class ALevelProcessController;

USTRUCT(BlueprintType)
struct FPushBoxFlowNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Flow")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Flow")
	TSoftObjectPtr<ALevelProcessController> ProcessController;
};

UCLASS(BlueprintType)
class PUSHBOX_API UPushBoxFlowDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PushBox|Flow")
	TArray<FPushBoxFlowNode> Nodes;
};
