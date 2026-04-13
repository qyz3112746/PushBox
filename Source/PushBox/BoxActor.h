// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoxActor.generated.h"

class ABoxTargetCell;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ABoxActor : public AActor
{
	GENERATED_BODY()

public:
	ABoxActor();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Box")
	void SetGridCoord(const FIntPoint& InGridCoord);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Box")
	void MoveToGridCoord(const FIntPoint& InGridCoord, const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Box")
	void SetBoxMesh(UStaticMesh* InMesh);

	UFUNCTION(BlueprintPure, Category = "PushBox|Box")
	FIntPoint GetGridCoord() const { return GridCoord; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushBox|Box")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushBox|Box")
	UStaticMeshComponent* BoxMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushBox|Rules")
	TSubclassOf<ABoxTargetCell> RequiredTargetCellClass;

private:
	UPROPERTY(VisibleAnywhere, Category = "PushBox|Box")
	FIntPoint GridCoord;
};
