// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridCellBase.h"
#include "BoxCell.generated.h"

class ABoxActor;
class ABoxTargetCell;
class UStaticMesh;

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

	// Mesh used by the spawned BoxActor (independent from cell's own mesh component).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "8"))
	TObjectPtr<UStaticMesh> BoxActorMesh;
};
