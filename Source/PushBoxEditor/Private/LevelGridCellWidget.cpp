// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelGridCellWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
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

void ULevelGridCellWidget::SetDisplayStyle(const FLinearColor& InColor, UTexture2D* InIcon)
{
	DisplayColor = InColor;
	DisplayIcon = InIcon;
	RefreshVisuals();
}

void ULevelGridCellWidget::SetSelectedState(bool bInSelected)
{
	bIsSelected = bInSelected;
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

FReply ULevelGridCellWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCellMouseReleased.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void ULevelGridCellWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (bIsValidCoord && FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton))
	{
		OnCellHoveredWhilePressed.Broadcast(Coord);
	}
}

FReply ULevelGridCellWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsValidCoord && FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton))
	{
		OnCellHoveredWhilePressed.Broadcast(Coord);
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
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
		FLinearColor EffectiveColor = bIsValidCoord ? DisplayColor : InvalidCellColor;
		if (bIsValidCoord && bIsSelected)
		{
			EffectiveColor = FLinearColor::LerpUsingHSV(EffectiveColor, SelectionOutlineColor, 0.35f);
		}

		const bool bUseBorderIconFallback = bIsValidCoord && DisplayIcon && !CellIconImage;
		if (bUseBorderIconFallback)
		{
			CellBorder->SetBrushFromTexture(DisplayIcon);
			CellBorder->SetBrushColor(FLinearColor::White);
		}
		else
		{
			CellBorder->SetBrushFromTexture(nullptr);
			CellBorder->SetBrushColor(EffectiveColor);
		}
	}

	if (SelectionBorder)
	{
		SelectionBorder->SetBrushColor(SelectionOutlineColor);
		SelectionBorder->SetVisibility((bIsValidCoord && bIsSelected) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CellIconImage)
	{
		if (bIsValidCoord && DisplayIcon)
		{
			CellIconImage->SetBrushFromTexture(DisplayIcon, true);
			CellIconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			CellIconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
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
