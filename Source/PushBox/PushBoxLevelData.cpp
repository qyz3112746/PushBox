// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelData.h"

void UPushBoxLevelData::CopyFromLevelData(const UPushBoxLevelData* SourceLevelData)
{
	if (!SourceLevelData || SourceLevelData == this)
	{
		return;
	}

	LevelId = SourceLevelData->LevelId;
	GridWidth = SourceLevelData->GridWidth;
	GridHeight = SourceLevelData->GridHeight;
	DefaultCellClass = SourceLevelData->DefaultCellClass;
	CellDefinitions = SourceLevelData->CellDefinitions;
}
