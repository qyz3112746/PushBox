// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PushBoxFlowDirector.generated.h"

class ALevelProcessController;
class UPushBoxFlowDataAsset;

UENUM(BlueprintType)
enum class EPushBoxFlowStartMode : uint8
{
	UseFlowDefault UMETA(DisplayName = "Use Flow Default"),
	DebugNodeLevel UMETA(DisplayName = "Debug Node Level"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPushBoxDirectorFlowCompletedSignature);

UCLASS(Blueprintable)
class PUSHBOX_API APushBoxFlowDirector : public AActor
{
	GENERATED_BODY()

public:
	APushBoxFlowDirector();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	TObjectPtr<UPushBoxFlowDataAsset> FlowDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	bool bAutoStartOnBeginPlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow")
	EPushBoxFlowStartMode StartMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow", meta = (ClampMin = "0"))
	int32 DebugNodeIndex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushBox|Flow", meta = (ClampMin = "0"))
	int32 DebugLevelIndexInNode;

	UPROPERTY(BlueprintAssignable, Category = "PushBox|Flow")
	FPushBoxDirectorFlowCompletedSignature OnFlowCompleted;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool StartFlow();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool ConfirmAdvance();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Flow")
	bool ConfirmReplayCurrent();

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	ALevelProcessController* GetActiveProcessController() const { return ActiveProcessController.Get(); }

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	int32 GetCurrentNodeIndex() const { return CurrentNodeIndex; }

	UFUNCTION(BlueprintPure, Category = "PushBox|Flow")
	int32 GetCurrentLevelIndexInNode() const { return CurrentLevelIndexInNode; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<ALevelProcessController> ActiveProcessController;

	int32 CurrentNodeIndex;
	int32 CurrentLevelIndexInNode;

	bool ActivateNodeLevel(int32 NodeIndex, int32 LevelIndexInNode);
	void BindActiveController(ALevelProcessController* Controller);
	UFUNCTION()
	void HandleActiveControllerFlowCompleted();
};
