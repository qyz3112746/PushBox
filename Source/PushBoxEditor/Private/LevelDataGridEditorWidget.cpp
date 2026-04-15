// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelDataGridEditorWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/Texture2D.h"
#include "GridCellBase.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "LevelGridCellWidget.h"

bool ULevelDataGridEditorWidget::LoadFromLevelData(UPushBoxLevelData* InData)
{
	TArray<FCellDisplay> EmptyList;
	return LoadFromLevelDataWithCellDisplay(InData, EmptyList);
}

bool ULevelDataGridEditorWidget::LoadFromLevelDataWithCellDisplay(UPushBoxLevelData* InData, TArray<FCellDisplay>& InOutCells)
{
	if (!InData)
	{
		return false;
	}

	EditingLevelData = InData;
	GridWidth = FMath::Max(1, EditingLevelData->GridWidth);
	GridHeight = FMath::Max(1, EditingLevelData->GridHeight);
	DefaultCellClass = EditingLevelData->DefaultCellClass;

	ResolvedCells.Empty();
	ResolvedCells.SetNum(GridWidth * GridHeight);
	for (int32 Index = 0; Index < ResolvedCells.Num(); ++Index)
	{
		ResolvedCells[Index] = DefaultCellClass;
	}

	for (const FPushBoxCellSpawnData& Def : EditingLevelData->CellDefinitions)
	{
		if (!IsCoordValid(Def.GridCoord))
		{
			continue;
		}

		ResolvedCells[ToIndex(Def.GridCoord)] = Def.CellClass;
	}

	SyncCellDisplayList(InOutCells);
	RebuildGrid();
	ResetView();
	return true;
}

bool ULevelDataGridEditorWidget::WriteBackToLevelData()
{
	if (!EditingLevelData)
	{
		return false;
	}

	EditingLevelData->GridWidth = FMath::Max(1, GridWidth);
	EditingLevelData->GridHeight = FMath::Max(1, GridHeight);
	EditingLevelData->DefaultCellClass = DefaultCellClass;

	EditingLevelData->CellDefinitions.Reset();

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const int32 Index = ToIndex(FIntPoint(X, Y));
			const TSubclassOf<AGridCellBase> CellClass = ResolvedCells.IsValidIndex(Index) ? ResolvedCells[Index] : nullptr;
			if (CellClass == DefaultCellClass)
			{
				continue;
			}

			FPushBoxCellSpawnData NewDef;
			NewDef.GridCoord = FIntPoint(X, Y);
			NewDef.CellClass = CellClass;
			EditingLevelData->CellDefinitions.Add(NewDef);
		}
	}

	return true;
}

void ULevelDataGridEditorWidget::SetMapSize(int32 InWidth, int32 InHeight)
{
	const int32 NewWidth = FMath::Max(1, InWidth);
	const int32 NewHeight = FMath::Max(1, InHeight);
	if (NewWidth == GridWidth && NewHeight == GridHeight)
	{
		return;
	}

	TArray<TSubclassOf<AGridCellBase>> OldResolved = ResolvedCells;
	const int32 OldWidth = GridWidth;
	const int32 OldHeight = GridHeight;

	GridWidth = NewWidth;
	GridHeight = NewHeight;

	ResolvedCells.Empty();
	ResolvedCells.SetNum(GridWidth * GridHeight);
	for (int32 Index = 0; Index < ResolvedCells.Num(); ++Index)
	{
		ResolvedCells[Index] = DefaultCellClass;
	}

	const int32 CopyWidth = FMath::Min(OldWidth, GridWidth);
	const int32 CopyHeight = FMath::Min(OldHeight, GridHeight);
	for (int32 Y = 0; Y < CopyHeight; ++Y)
	{
		for (int32 X = 0; X < CopyWidth; ++X)
		{
			const int32 OldIndex = Y * OldWidth + X;
			const int32 NewIndex = Y * GridWidth + X;
			if (OldResolved.IsValidIndex(OldIndex) && ResolvedCells.IsValidIndex(NewIndex))
			{
				ResolvedCells[NewIndex] = OldResolved[OldIndex];
			}
		}
	}

	RebuildGrid();
	ResetView();
}

void ULevelDataGridEditorWidget::SetActiveBrushCellClass(TSubclassOf<AGridCellBase> InClass)
{
	ActiveBrushCellClass = InClass;
}

void ULevelDataGridEditorWidget::ClearToDefault()
{
	for (int32 Index = 0; Index < ResolvedCells.Num(); ++Index)
	{
		ResolvedCells[Index] = DefaultCellClass;
	}

	RefreshAllCells();
}

void ULevelDataGridEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	CurrentZoom = FMath::Clamp(CurrentZoom, ZoomMin, ZoomMax);
	UpdateContentBaseSize();
	ApplyTransform();
}

void ULevelDataGridEditorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const FVector2D ViewportSize = GetViewportSize();
	if (!ViewportSize.Equals(LastViewportSize, 0.5f))
	{
		LastViewportSize = ViewportSize;
		if (bAutoFitOnReset && !bHasUserAdjustedView)
		{
			AutoFitView(true);
		}
		else
		{
			ClampPanOffset();
			ApplyTransform();
		}
	}
}

FReply ULevelDataGridEditorWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bIsPanning = true;
		LastMouseScreenPos = InMouseEvent.GetScreenSpacePosition();
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULevelDataGridEditorWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bIsPanning)
	{
		bIsPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply ULevelDataGridEditorWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsPanning)
	{
		const FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
		const FVector2D Delta = MousePos - LastMouseScreenPos;
		LastMouseScreenPos = MousePos;

		if (!Delta.IsNearlyZero())
		{
			bHasUserAdjustedView = true;
			PanOffset += Delta;
			ClampPanOffset();
			ApplyTransform();
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply ULevelDataGridEditorWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const float WheelDelta = InMouseEvent.GetWheelDelta();
	if (FMath::IsNearlyZero(WheelDelta))
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	const float OldZoom = CurrentZoom;
	const float ZoomFactor = WheelDelta > 0.0f ? ZoomStep : (1.0f / ZoomStep);
	CurrentZoom = FMath::Clamp(CurrentZoom * ZoomFactor, ZoomMin, ZoomMax);
	if (FMath::IsNearlyEqual(OldZoom, CurrentZoom))
	{
		return FReply::Handled();
	}
	bHasUserAdjustedView = true;

	const FVector2D MouseLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D ContentPosBeforeZoom = (MouseLocal - PanOffset) / OldZoom;
	PanOffset = MouseLocal - (ContentPosBeforeZoom * CurrentZoom);

	ClampPanOffset();
	ApplyTransform();
	return FReply::Handled();
}

void ULevelDataGridEditorWidget::HandleCellClicked(FIntPoint Coord)
{
	if (!IsCoordValid(Coord))
	{
		return;
	}

	const int32 Index = ToIndex(Coord);
	if (!ResolvedCells.IsValidIndex(Index))
	{
		return;
	}

	ResolvedCells[Index] = ActiveBrushCellClass;
	if (SpawnedCellWidgets.IsValidIndex(Index) && SpawnedCellWidgets[Index])
	{
		SpawnedCellWidgets[Index]->SetCellData(Coord, ResolvedCells[Index], true);
	}
}

void ULevelDataGridEditorWidget::RebuildGrid()
{
	if (!GridPanel)
	{
		return;
	}

	GridPanel->ClearChildren();
	SpawnedCellWidgets.Reset();
	SpawnedCellWidgets.SetNum(GridWidth * GridHeight);

	TSubclassOf<ULevelGridCellWidget> EffectiveCellWidgetClass = CellWidgetClass;
	if (!EffectiveCellWidgetClass)
	{
		EffectiveCellWidgetClass = ULevelGridCellWidget::StaticClass();
	}

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			ULevelGridCellWidget* CellWidget = CreateWidget<ULevelGridCellWidget>(this, EffectiveCellWidgetClass);
			if (!CellWidget)
			{
				continue;
			}

			const FIntPoint Coord(X, Y);
			const int32 Index = ToIndex(Coord);
			const TSubclassOf<AGridCellBase> CellClass = ResolvedCells.IsValidIndex(Index) ? ResolvedCells[Index] : DefaultCellClass;

			CellWidget->SetCellVisualSize(CellPixelSize);
			CellWidget->SetCellData(Coord, CellClass, true);
			if (const FCellDisplay* Display = CellDisplayLookup.Find(CellClass))
			{
				CellWidget->SetDisplayStyle(Display->BGColor, Display->Icon);
			}
			else
			{
				CellWidget->SetDisplayStyle(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f), nullptr);
			}
			CellWidget->OnCellClicked.AddDynamic(this, &ULevelDataGridEditorWidget::HandleCellClicked);

			if (UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(CellWidget, Y, X))
			{
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}

			if (SpawnedCellWidgets.IsValidIndex(Index))
			{
				SpawnedCellWidgets[Index] = CellWidget;
			}
		}
	}

	UpdateContentBaseSize();
	ClampPanOffset();
	ApplyTransform();
}

void ULevelDataGridEditorWidget::RefreshAllCells()
{
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FIntPoint Coord(X, Y);
			const int32 Index = ToIndex(Coord);
			if (!SpawnedCellWidgets.IsValidIndex(Index) || !SpawnedCellWidgets[Index])
			{
				continue;
			}

			const TSubclassOf<AGridCellBase> CellClass = ResolvedCells.IsValidIndex(Index) ? ResolvedCells[Index] : DefaultCellClass;
			SpawnedCellWidgets[Index]->SetCellData(Coord, CellClass, true);
			if (const FCellDisplay* Display = CellDisplayLookup.Find(CellClass))
			{
				SpawnedCellWidgets[Index]->SetDisplayStyle(Display->BGColor, Display->Icon);
			}
			else
			{
				SpawnedCellWidgets[Index]->SetDisplayStyle(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f), nullptr);
			}
		}
	}
}

void ULevelDataGridEditorWidget::ApplyTransform()
{
	UWidget* TransformWidget = GridCanvas ? Cast<UWidget>(GridCanvas) : Cast<UWidget>(GridPanel);
	if (!TransformWidget)
	{
		return;
	}

	FWidgetTransform Transform = TransformWidget->GetRenderTransform();
	Transform.Translation = PanOffset;
	Transform.Scale = FVector2D(CurrentZoom, CurrentZoom);
	TransformWidget->SetRenderTransform(Transform);
	TransformWidget->SetRenderTransformPivot(FVector2D::ZeroVector);
}

void ULevelDataGridEditorWidget::ClampPanOffset()
{
	if (!bClampPan)
	{
		return;
	}

	const FVector2D ViewSize = GetViewportSize();
	const FVector2D ScaledContentSize = ContentBaseSize * CurrentZoom;

	auto ClampAxis = [](double ViewAxis, double ContentAxis, double& InOutPan)
	{
		if (ContentAxis <= ViewAxis)
		{
			InOutPan = (ViewAxis - ContentAxis) * 0.5;
		}
		else
		{
			const double MinPan = ViewAxis - ContentAxis;
			const double MaxPan = 0.0;
			InOutPan = FMath::Clamp(InOutPan, MinPan, MaxPan);
		}
	};

	double PanX = PanOffset.X;
	double PanY = PanOffset.Y;
	ClampAxis(ViewSize.X, ScaledContentSize.X, PanX);
	ClampAxis(ViewSize.Y, ScaledContentSize.Y, PanY);
	PanOffset = FVector2D(PanX, PanY);
}

void ULevelDataGridEditorWidget::ResetView()
{
	bHasUserAdjustedView = false;
	if (bAutoFitOnReset)
	{
		AutoFitView(true);
		return;
	}

	CurrentZoom = FMath::Clamp(1.0f, ZoomMin, ZoomMax);
	PanOffset = FVector2D::ZeroVector;
	ClampPanOffset();
	ApplyTransform();
}

float ULevelDataGridEditorWidget::ComputeFitZoom() const
{
	const FVector2D ViewportSize = GetViewportSize();
	if (ViewportSize.X <= 1.0f || ViewportSize.Y <= 1.0f || ContentBaseSize.X <= 1.0f || ContentBaseSize.Y <= 1.0f)
	{
		return FMath::Clamp(1.0f, ZoomMin, ZoomMax);
	}

	const float FitX = ViewportSize.X / ContentBaseSize.X;
	const float FitY = ViewportSize.Y / ContentBaseSize.Y;
	const float FitZoom = FMath::Min(FitX, FitY);
	const float SafePadding = FMath::Clamp(FitPaddingPercent, 0.0f, 0.9f);
	return FMath::Clamp(FitZoom * (1.0f - SafePadding), ZoomMin, ZoomMax);
}

void ULevelDataGridEditorWidget::AutoFitView(bool bResetPan)
{
	CurrentZoom = ComputeFitZoom();
	if (bResetPan)
	{
		PanOffset = FVector2D::ZeroVector;
	}

	ClampPanOffset();
	ApplyTransform();
}

FVector2D ULevelDataGridEditorWidget::GetViewportSize() const
{
	const FVector2D CachedSize = GetCachedGeometry().GetLocalSize();
	if (CachedSize.X > 1.0f && CachedSize.Y > 1.0f)
	{
		return CachedSize;
	}

	return ContentBaseSize.GetMax() > 0.0f ? ContentBaseSize : FVector2D(1.0f, 1.0f);
}

int32 ULevelDataGridEditorWidget::ToIndex(const FIntPoint& Coord) const
{
	return Coord.Y * GridWidth + Coord.X;
}

bool ULevelDataGridEditorWidget::IsCoordValid(const FIntPoint& Coord) const
{
	return Coord.X >= 0 && Coord.Y >= 0 && Coord.X < GridWidth && Coord.Y < GridHeight;
}

void ULevelDataGridEditorWidget::UpdateContentBaseSize()
{
	ContentBaseSize = FVector2D(GridWidth * CellPixelSize, GridHeight * CellPixelSize);
}

void ULevelDataGridEditorWidget::SyncCellDisplayList(TArray<FCellDisplay>& InOutCells)
{
	TSet<TSubclassOf<AGridCellBase>> UniqueCellTypes;
	for (const TSubclassOf<AGridCellBase>& CellClass : ResolvedCells)
	{
		if (CellClass)
		{
			UniqueCellTypes.Add(CellClass);
		}
	}

	TSet<TSubclassOf<AGridCellBase>> ExistingTypes;
	for (const FCellDisplay& Display : InOutCells)
	{
		if (Display.CellType)
		{
			ExistingTypes.Add(Display.CellType);
		}
	}

	for (const TSubclassOf<AGridCellBase>& CellType : UniqueCellTypes)
	{
		if (!ExistingTypes.Contains(CellType))
		{
			InOutCells.Add(BuildRandomCellDisplay(CellType));
		}
	}

	RebuildDisplayLookupFromArray(InOutCells);
}

FCellDisplay ULevelDataGridEditorWidget::BuildRandomCellDisplay(TSubclassOf<AGridCellBase> CellType) const
{
	FCellDisplay NewDisplay;
	NewDisplay.CellType = CellType;
	NewDisplay.Icon = nullptr;

	const float Hue = FMath::FRandRange(0.0f, 360.0f);
	const float Saturation = FMath::FRandRange(0.55f, 0.85f);
	const float Value = FMath::FRandRange(0.70f, 0.95f);
	NewDisplay.BGColor = FLinearColor::MakeFromHSV8(
		static_cast<uint8>(Hue / 360.0f * 255.0f),
		static_cast<uint8>(Saturation * 255.0f),
		static_cast<uint8>(Value * 255.0f));
	return NewDisplay;
}

void ULevelDataGridEditorWidget::RebuildDisplayLookupFromArray(const TArray<FCellDisplay>& InCells)
{
	CellDisplayLookup.Reset();
	for (const FCellDisplay& Display : InCells)
	{
		if (Display.CellType)
		{
			CellDisplayLookup.Add(Display.CellType, Display);
		}
	}
}
