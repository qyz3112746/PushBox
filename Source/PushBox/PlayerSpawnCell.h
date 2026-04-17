// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridCellBase.h"
#include "PlayerSpawnCell.generated.h"

UCLASS(Blueprintable)
class APlayerSpawnCell : public AGridCellBase
{
	GENERATED_BODY()

protected:
	virtual void BuildEditorPreview_Implementation(FCellEditorPreviewContext& PreviewContext) override;
};
