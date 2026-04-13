// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PushBoxPlayerController.generated.h"

class APushBoxLevelRuntime;
class ALevelProcessController;

DECLARE_LOG_CATEGORY_EXTERN(LogPushBoxController, Log, All);

UCLASS()
class APushBoxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APushBoxPlayerController();

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;

private:
	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();
	void RestartLevel();
	FIntPoint GetCameraForwardGridDirection() const;
	FIntPoint GetCameraRightGridDirection() const;
	static FIntPoint ToCardinalGridDirection(const FVector& WorldDirection);

	bool TryMove(const FIntPoint& Direction);
	ALevelProcessController* ResolveProcessController() const;
	APushBoxLevelRuntime* ResolveLevelRuntime() const;

	mutable TObjectPtr<ALevelProcessController> CachedProcessController;
	mutable TObjectPtr<APushBoxLevelRuntime> CachedLevelRuntime;
};


