// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerSpawnCell.h"

void APlayerSpawnCell::BuildEditorPreview_Implementation(FCellEditorPreviewContext& PreviewContext)
{
	SpawnPreviewPawn(PreviewContext, PreviewContext.PreviewPawnClass);
}
