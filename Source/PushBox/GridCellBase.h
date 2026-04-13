// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridCellBase.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS(Blueprintable, meta = (PrioritizeCategories = "Cell"))
class AGridCellBase : public AActor
{
	GENERATED_BODY()

public:
	AGridCellBase();

	UFUNCTION(BlueprintCallable, Category = "Cell")
	void SetGridCoord(const FIntPoint& InGridCoord);

	UFUNCTION(BlueprintPure, Category = "Cell")
	FIntPoint GetGridCoord() const { return GridCoord; }

	UFUNCTION(BlueprintPure, Category = "Cell")
	bool IsBlockingMovement() const { return bBlocksMovement; }

	UFUNCTION(BlueprintCallable, Category = "Cell")
	void SetBlocksMovement(bool bInBlocksMovement);

	UFUNCTION(BlueprintPure, Category = "Cell")
	UStaticMesh* GetCellStaticMesh() const;

	UFUNCTION(BlueprintPure, Category = "Cell")
	FTransform GetCellMeshRelativeTransform() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "1"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "2"))
	UStaticMeshComponent* CellMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "3"))
	UNiagaraComponent* CellNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell", meta = (DisplayPriority = "4"))
	bool bBlocksMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "5"))
	FIntPoint GridCoord;
};
