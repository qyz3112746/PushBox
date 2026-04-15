// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelGridCellWidget.h"

#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GridCellBase.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void ULevelGridCellWidget::SetCellData(const FIntPoint& InCoord, TSubclassOf<AGridCellBase> InCellClass, bool bInIsValidCoord)
{
	Coord = InCoord;
	CellClass = InCellClass;
	bIsValidCoord = bInIsValidCoord;
	RefreshVisuals();
}

void ULevelGridCellWidget::SetCellVisualSize(float InSize)
{
	if (!CellSizeBox)
	{
		return;
	}

	const float ClampedSize = FMath::Max(1.0f, InSize);
	CellSizeBox->SetWidthOverride(ClampedSize);
	CellSizeBox->SetHeightOverride(ClampedSize);
}

FReply ULevelGridCellWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsValidCoord)
	{
		OnCellClicked.Broadcast(Coord);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULevelGridCellWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshVisuals();
}

void ULevelGridCellWidget::RefreshVisuals()
{
	if (CellBorder)
	{
		CellBorder->SetBrushColor(bIsValidCoord ? ValidCellColor : InvalidCellColor);
	}

	if (CellLabel)
	{
		if (bShowClassNameInLabel && CellClass)
		{
			CellLabel->SetText(FText::FromString(CellClass->GetName()));
		}
		else
		{
			CellLabel->SetText(FText::GetEmpty());
		}
	}

	SetIsEnabled(bIsValidCoord);
	BP_OnCellVisualRefresh();
}
