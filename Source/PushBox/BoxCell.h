// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridCellBase.h"
#include "BoxCell.generated.h"

UCLASS(Blueprintable)
class ABoxCell : public AGridCellBase
{
	GENERATED_BODY()

public:
	ABoxCell();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cell|Box", meta = (DisplayPriority = "6"))
	TSubclassOf<ABoxActor> BoxSpawnClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cell|Box", meta = (DisplayPriority = "7"))
	TObjectPtr<ABoxActor> SpawnedBox;

protected:
	virtual void BeginPlay() override;
	virtual void BuildEditorPreview_Implementation(FCellEditorPreviewContext& PreviewContext) override;
};
