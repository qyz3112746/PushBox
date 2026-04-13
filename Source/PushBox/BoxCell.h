// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridCellBase.h"
#include "BoxCell.generated.h"

class ABoxActor;
class ABoxTargetCell;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ABoxCell : public AGridCellBase
{
	GENERATED_BODY()

public:
	ABoxCell();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "6"))
	TSubclassOf<ABoxTargetCell> TargetCellClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "7"))
	TSubclassOf<ABoxActor> BoxActorClass;

	// Editor-visible preview mesh for configuring spawned box visuals.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "8"))
	TObjectPtr<UStaticMeshComponent> BoxPreviewMeshComponent;

	UFUNCTION(BlueprintPure, Category = "Cell")
	UStaticMesh* GetBoxPreviewMesh() const;

	UFUNCTION(BlueprintPure, Category = "Cell")
	FTransform GetBoxPreviewMeshRelativeTransform() const;
};
