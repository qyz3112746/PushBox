// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "SaveDataTypes.h"
#include "PushBoxEditorBridgeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApplyCellDisplayRequested, const FCellDisplay&, Payload, const FString&, PropertyPath);

UCLASS()
class PUSHBOXEDITOR_API UPushBoxEditorBridgeSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "PushBox|LevelTool")
	FOnApplyCellDisplayRequested OnApplyCellDisplayRequested;

	UFUNCTION(BlueprintCallable, Category = "PushBox|LevelTool")
	void BroadcastApplyCellDisplayRequested(const FCellDisplay& Payload, const FString& PropertyPath);
};
