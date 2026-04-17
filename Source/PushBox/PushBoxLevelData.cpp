// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelData.h"
#include "GridCellBase.h"

namespace
{
	static int32 ClampGridDimension(const int32 InValue)
	{
		return FMath::Max(1, InValue);
	}
}

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
	GridRows = SourceLevelData->GridRows;
	NormalizeGrid(DefaultCellClass);
}

void UPushBoxLevelData::InitializeGrid(int32 InWidth, int32 InHeight, TSubclassOf<AGridCellBase> FillCellClass)
{
	GridWidth = ClampGridDimension(InWidth);
	GridHeight = ClampGridDimension(InHeight);

	GridRows.Reset();
	GridRows.SetNum(GridHeight);
	for (FPushBoxLevelGridRow& Row : GridRows)
	{
		Row.Cells.SetNum(GridWidth);
		for (int32 X = 0; X < GridWidth; ++X)
		{
			Row.Cells[X] = FillCellClass;
		}
	}
}

void UPushBoxLevelData::ResizeGrid(int32 NewWidth, int32 NewHeight, TSubclassOf<AGridCellBase> FillCellClass)
{
	const int32 TargetWidth = ClampGridDimension(NewWidth);
	const int32 TargetHeight = ClampGridDimension(NewHeight);

	TArray<FPushBoxLevelGridRow> OldRows = GridRows;
	const int32 OldHeight = GridHeight;
	const int32 OldWidth = GridWidth;

	GridWidth = TargetWidth;
	GridHeight = TargetHeight;
	GridRows.Reset();
	GridRows.SetNum(GridHeight);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		FPushBoxLevelGridRow& NewRow = GridRows[Y];
		NewRow.Cells.SetNum(GridWidth);
		for (int32 X = 0; X < GridWidth; ++X)
		{
			TSubclassOf<AGridCellBase> NewValue = FillCellClass;
			if (Y < OldHeight && OldRows.IsValidIndex(Y))
			{
				const FPushBoxLevelGridRow& OldRow = OldRows[Y];
				if (X < OldWidth && OldRow.Cells.IsValidIndex(X))
				{
					NewValue = OldRow.Cells[X];
				}
			}
			NewRow.Cells[X] = NewValue;
		}
	}
}

TSubclassOf<AGridCellBase> UPushBoxLevelData::GetCellAt(FIntPoint GridCoord) const
{
	if (GridCoord.X < 0 || GridCoord.Y < 0 || GridCoord.X >= GridWidth || GridCoord.Y >= GridHeight)
	{
		return nullptr;
	}

	if (!GridRows.IsValidIndex(GridCoord.Y))
	{
		return nullptr;
	}

	const FPushBoxLevelGridRow& Row = GridRows[GridCoord.Y];
	if (!Row.Cells.IsValidIndex(GridCoord.X))
	{
		return nullptr;
	}

	return Row.Cells[GridCoord.X];
}

bool UPushBoxLevelData::SetCellAt(FIntPoint GridCoord, TSubclassOf<AGridCellBase> CellClass)
{
	if (GridCoord.X < 0 || GridCoord.Y < 0 || GridCoord.X >= GridWidth || GridCoord.Y >= GridHeight)
	{
		return false;
	}

	if (!GridRows.IsValidIndex(GridCoord.Y))
	{
		return false;
	}

	FPushBoxLevelGridRow& Row = GridRows[GridCoord.Y];
	if (!Row.Cells.IsValidIndex(GridCoord.X))
	{
		return false;
	}

	Row.Cells[GridCoord.X] = CellClass;
	return true;
}

void UPushBoxLevelData::NormalizeGrid(TSubclassOf<AGridCellBase> FillCellClass)
{
	GridWidth = ClampGridDimension(GridWidth);
	GridHeight = ClampGridDimension(GridHeight);

	const int32 OldHeight = GridRows.Num();
	GridRows.SetNum(GridHeight);
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		FPushBoxLevelGridRow& Row = GridRows[Y];
		const int32 OldWidth = Row.Cells.Num();
		Row.Cells.SetNum(GridWidth);
		if (OldWidth < GridWidth)
		{
			for (int32 X = OldWidth; X < GridWidth; ++X)
			{
				Row.Cells[X] = FillCellClass;
			}
		}
	}

	if (OldHeight < GridHeight)
	{
		for (int32 Y = OldHeight; Y < GridHeight; ++Y)
		{
			FPushBoxLevelGridRow& Row = GridRows[Y];
			for (int32 X = 0; X < GridWidth; ++X)
			{
				Row.Cells[X] = FillCellClass;
			}
		}
	}
}
