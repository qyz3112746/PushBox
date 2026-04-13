// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoxActor.generated.h"
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ABoxActor : public AActor
{
	GENERATED_BODY()

public:
	ABoxActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "PushBox|Box")
	void SetGridCoord(const FIntPoint& InGridCoord);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Box")
	void MoveToGridCoord(const FIntPoint& InGridCoord, const FVector& WorldLocation, float InMoveDuration = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "PushBox|Box")
	void SetBoxMesh(UStaticMesh* InMesh);

	UFUNCTION(BlueprintPure, Category = "PushBox|Box")
	FIntPoint GetGridCoord() const { return GridCoord; }

	UFUNCTION(BlueprintCallable, Category = "PushBox|Box")
	void SetTargetCoord(const FIntPoint& InTargetCoord);

	UFUNCTION(BlueprintPure, Category = "PushBox|Box")
	FIntPoint GetTargetCoord() const { return TargetCoord; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushBox|Box")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushBox|Box")
	UStaticMeshComponent* BoxMeshComponent;

private:
	UPROPERTY(VisibleAnywhere, Category = "PushBox|Box")
	FIntPoint GridCoord;

	UPROPERTY(VisibleAnywhere, Category = "PushBox|Box")
	FIntPoint TargetCoord;

	bool bIsInterpolatingMove;
	FVector MoveStartLocation;
	FVector MoveTargetLocation;
	float MoveStartTime;
	float MoveDuration;
};
