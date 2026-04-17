// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PushBoxLevelPreviewSubsystem.generated.h"

class ALevelProcessController;
class UPushBoxLevelData;

UCLASS()
class PUSHBOXEDITOR_API UPushBoxLevelPreviewSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PushBox|Preview")
	bool StartPreview(ALevelProcessController* InController, UPushBoxLevelData* InLevelData, int32 InSequenceIndex);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Preview")
	void ClearPreview();

	UFUNCTION(BlueprintPure, Category = "PushBox|Preview")
	bool HasActivePreview() const { return SpawnedPreviewActors.Num() > 0; }

private:
	TSubclassOf<class APawn> ResolvePreviewPawnClass(UWorld* InWorld) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ALevelProcessController> ActiveProcessController;

	UPROPERTY(Transient)
	TObjectPtr<UPushBoxLevelData> ActiveLevelData;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedPreviewActors;
};
