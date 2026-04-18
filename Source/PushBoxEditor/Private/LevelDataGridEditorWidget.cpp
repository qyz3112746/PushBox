// Copyright Epic Games, Inc. All Rights Reserved.

#include "LevelDataGridEditorWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "ContentBrowserModule.h"
#include "Engine/Texture2D.h"
#include "Editor.h"
#include "GridCellBase.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "IContentBrowserSingleton.h"
#include "LevelGridCellWidget.h"
#include "PushBoxEditorBridgeSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace
{
	static constexpr int32 FixedGridCapacity = 64;
}

bool ULevelDataGridEditorWidget::LoadFromLevelDataWithCellDisplay(UPushBoxLevelData* InData, TArray<FCellDisplay>& InOutCells)
{
	if (!InData)
	{
		return false;
	}

	EditingLevelData = InData;
	TemporaryLevelData = DuplicateObject<UPushBoxLevelData>(InData, this);
	if (!TemporaryLevelData)
	{
		return false;
	}

	SelectedIndices.Reset();
	SelectionBaseIndices.Reset();
	bIsSelecting = false;
	return ReloadMapDataWithoutRebuild(InData, InOutCells);
}

bool ULevelDataGridEditorWidget::SaveTemporaryLevelDataAsAssetWithDialog()
{
	if (!TemporaryLevelData)
	{
		return false;
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FSaveAssetDialogConfig SaveConfig;
	SaveConfig.DialogTitleOverride = FText::FromString(TEXT("Save PushBox Level Data Asset"));
	SaveConfig.DefaultPath = TEXT("/Game");
	SaveConfig.DefaultAssetName = !TemporaryLevelData->LevelId.IsNone()
		? FString::Printf(TEXT("DA_%s"), *TemporaryLevelData->LevelId.ToString())
		: TEXT("DA_NewLevel");
	SaveConfig.AssetClassNames.Add(UPushBoxLevelData::StaticClass()->GetClassPathName());
	SaveConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

	const FString ObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveConfig);
	if (ObjectPath.IsEmpty())
	{
		return false;
	}

	if (!FPackageName::IsValidObjectPath(ObjectPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveTemporaryLevelDataAsAssetWithDialog failed: invalid object path '%s'."), *ObjectPath);
		return false;
	}

	const FSoftObjectPath SavedObjectPath(ObjectPath);
	const FString PackageName = SavedObjectPath.GetLongPackageName();
	const FString AssetName = SavedObjectPath.GetAssetName();
	if (!FPackageName::IsValidLongPackageName(PackageName) || AssetName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveTemporaryLevelDataAsAssetWithDialog failed: invalid package/object from path '%s'."), *ObjectPath);
		return false;
	}
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return false;
	}

	UPushBoxLevelData* SavedAsset = FindObject<UPushBoxLevelData>(Package, *AssetName);
	const bool bNewAsset = (SavedAsset == nullptr);
	if (!SavedAsset)
	{
		SavedAsset = NewObject<UPushBoxLevelData>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	}
	if (!SavedAsset)
	{
		return false;
	}

	SavedAsset->CopyFromLevelData(TemporaryLevelData);
	SavedAsset->MarkPackageDirty();
	Package->MarkPackageDirty();

	if (bNewAsset)
	{
		FAssetRegistryModule::AssetCreated(SavedAsset);
	}

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_None;
	const bool bSaveSucceeded = UPackage::SavePackage(Package, SavedAsset, *PackageFileName, SaveArgs);
	if (!bSaveSucceeded)
	{
		return false;
	}

	OnLevelDataAssetSaved.Broadcast(SavedAsset);
	return true;
}

bool ULevelDataGridEditorWidget::RefreshLevelDisplayWithCellDisplay(UPushBoxLevelData* InData, TArray<FCellDisplay>& InOutCells)
{
	if (!InData)
	{
		return false;
	}

	if (!TemporaryLevelData)
	{
		return LoadFromLevelDataWithCellDisplay(InData, InOutCells);
	}
	return ReloadMapDataWithoutRebuild(InData, InOutCells);
}

bool ULevelDataGridEditorWidget::WriteBackToLevelData()
{
	if (!EditingLevelData || !TemporaryLevelData)
	{
		return false;
	}

	SyncTemporaryLevelDataFromResolved();

	EditingLevelData->GridWidth = TemporaryLevelData->GridWidth;
	EditingLevelData->GridHeight = TemporaryLevelData->GridHeight;
	EditingLevelData->DefaultCellClass = TemporaryLevelData->DefaultCellClass;
	EditingLevelData->GridRows = TemporaryLevelData->GridRows;

	return true;
}

void ULevelDataGridEditorWidget::SetMapSize(int32 InWidth, int32 InHeight)
{
	const int32 NewWidth = FMath::Clamp(InWidth, 0, FixedGridCapacity);
	const int32 NewHeight = FMath::Clamp(InHeight, 0, FixedGridCapacity);
	if (NewWidth == GridWidth && NewHeight == GridHeight)
	{
		return;
	}

	if (InWidth > FixedGridCapacity || InHeight > FixedGridCapacity)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetMapSize rejected: requested size %dx%d exceeds fixed capacity %dx%d."),
			InWidth, InHeight, FixedGridCapacity, FixedGridCapacity);
		return;
	}

	TArray<TSubclassOf<AGridCellBase>> OldResolved = ResolvedCells;
	const int32 OldWidth = GridWidth;
	const int32 OldHeight = GridHeight;

	GridWidth = NewWidth;
	GridHeight = NewHeight;

	ResolvedCells.Reset();
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

	InitializeFixedGridIfNeeded();
	ApplyValidRectVisibilityAndHitTest();
	ResetView();
	TSet<int32> NewSelection;
	for (const int32 Index : SelectedIndices)
	{
		if (Index >= 0 && Index < (GridWidth * GridHeight))
		{
			NewSelection.Add(Index);
		}
	}
	SelectedIndices = MoveTemp(NewSelection);
	SelectionBaseIndices = SelectedIndices;
	RefreshAllCells();
	SyncTemporaryLevelDataFromResolved();
}

bool ULevelDataGridEditorWidget::ApplyCellDisplayToSelection(const FCellDisplay& InDisplay)
{
	if (SelectedIndices.Num() == 0 || !InDisplay.CellType)
	{
		return false;
	}

	const TSet<int32> AppliedSelection = SelectedIndices;
	bool bAppliedAny = false;
	for (const int32 Index : AppliedSelection)
	{
		if (!ResolvedCells.IsValidIndex(Index))
		{
			continue;
		}

		ResolvedCells[Index] = InDisplay.CellType;
		RefreshWidgetAtIndex(Index);
		bAppliedAny = true;
	}

	if (!bAppliedAny)
	{
		return false;
	}

	bool bUpdatedDisplay = false;
	for (FCellDisplay& Display : TemporaryCellDisplayList)
	{
		if (Display.CellType == InDisplay.CellType)
		{
			Display = InDisplay;
			bUpdatedDisplay = true;
			break;
		}
	}
	if (!bUpdatedDisplay)
	{
		TemporaryCellDisplayList.Add(InDisplay);
	}
	RebuildDisplayLookupFromArray(TemporaryCellDisplayList);

	for (const int32 Index : AppliedSelection)
	{
		RefreshWidgetAtIndex(Index);
	}
	SyncTemporaryLevelDataFromResolved();

	SelectedIndices.Reset();
	SelectionBaseIndices.Reset();
	RefreshSelectionVisuals(AppliedSelection);
	return true;
}

void ULevelDataGridEditorWidget::ClearSelection()
{
	const TSet<int32> OldSelection = SelectedIndices;
	SelectedIndices.Reset();
	SelectionBaseIndices.Reset();
	RefreshSelectionVisuals(OldSelection);
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
	SyncTemporaryLevelDataFromResolved();
}

void ULevelDataGridEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);
	SetIsFocusable(true);

	if (GEditor)
	{
		if (UPushBoxEditorBridgeSubsystem* Bridge = GEditor->GetEditorSubsystem<UPushBoxEditorBridgeSubsystem>())
		{
			Bridge->RegisterActiveMapEditor(this);
		}
	}

	CurrentZoom = FMath::Clamp(CurrentZoom, ZoomMin, ZoomMax);
	EnsureSelectionMarqueeWidget();
	InitializeFixedGridIfNeeded();
	ApplyValidRectVisibilityAndHitTest();
	UpdateContentBaseSize();
	ApplyTransform();
}

void ULevelDataGridEditorWidget::NativeDestruct()
{
	if (GEditor)
	{
		if (UPushBoxEditorBridgeSubsystem* Bridge = GEditor->GetEditorSubsystem<UPushBoxEditorBridgeSubsystem>())
		{
			Bridge->UnregisterActiveMapEditor(this);
		}
	}

	Super::NativeDestruct();
}

FReply ULevelDataGridEditorWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void ULevelDataGridEditorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsSelecting && FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton))
	{
		const FVector2D CursorScreenPos = FSlateApplication::Get().GetCursorPos();
		FIntPoint HoverCoord = FIntPoint::ZeroValue;
		if (TryGetCoordFromPointer(MyGeometry, CursorScreenPos, HoverCoord) && HoverCoord != SelectionCurrentCoord)
		{
			SelectionCurrentCoord = HoverCoord;
			bSelectionMoved = true;
			UpdateSelectionFromDragRect();
			UpdateSelectionMarqueeVisual();
			Invalidate(EInvalidateWidgetReason::Paint);
		}
	}
	else if (bIsSelecting && !FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton))
	{
		EndSelection(true);
		Invalidate(EInvalidateWidgetReason::Paint);
	}

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

int32 ULevelDataGridEditorWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 SuperLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const float EffectiveZoom = FMath::Max(CurrentZoom, KINDA_SMALL_NUMBER);
	const float OutlineThickness = FMath::Max(1.0f, SelectedCellOutlineThickness);

	// Draw selected-cell outlines (always visible, independent of cell fill color).
	for (const int32 Index : SelectedIndices)
	{
		if (Index < 0 || Index >= (GridWidth * GridHeight))
		{
			continue;
		}

		const int32 DataX = Index % GridWidth;
		const int32 DataY = Index / GridWidth;
		const FIntPoint ViewCoord = DataToViewCoord(FIntPoint(DataX, DataY));
		if (!IsViewCoordValid(ViewCoord))
		{
			continue;
		}

		const FVector2D RectPos(
			PanOffset.X + (static_cast<float>(ViewCoord.X) * CellPixelSize * EffectiveZoom),
			PanOffset.Y + (static_cast<float>(ViewCoord.Y) * CellPixelSize * EffectiveZoom));
		const FVector2D RectSize(
			CellPixelSize * EffectiveZoom,
			CellPixelSize * EffectiveZoom);

		const FVector2D TopSize(RectSize.X, OutlineThickness);
		const FVector2D BottomPos(RectPos.X, RectPos.Y + RectSize.Y - OutlineThickness);
		const FVector2D SideSize(OutlineThickness, RectSize.Y);
		const FVector2D RightPos(RectPos.X + RectSize.X - OutlineThickness, RectPos.Y);

		FSlateDrawElement::MakeBox(
			OutDrawElements, SuperLayer + 1, AllottedGeometry.ToPaintGeometry(RectPos, TopSize),
			FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, SelectedCellOutlineColor);
		FSlateDrawElement::MakeBox(
			OutDrawElements, SuperLayer + 1, AllottedGeometry.ToPaintGeometry(BottomPos, TopSize),
			FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, SelectedCellOutlineColor);
		FSlateDrawElement::MakeBox(
			OutDrawElements, SuperLayer + 1, AllottedGeometry.ToPaintGeometry(RectPos, SideSize),
			FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, SelectedCellOutlineColor);
		FSlateDrawElement::MakeBox(
			OutDrawElements, SuperLayer + 1, AllottedGeometry.ToPaintGeometry(RightPos, SideSize),
			FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, SelectedCellOutlineColor);
	}

	if (!bIsSelecting || (!bSelectionMoved && SelectionStartCoord == SelectionCurrentCoord))
	{
		return SuperLayer + 1;
	}

	const int32 MinX = FMath::Min(SelectionStartCoord.X, SelectionCurrentCoord.X);
	const int32 MaxX = FMath::Max(SelectionStartCoord.X, SelectionCurrentCoord.X);
	const int32 MinY = FMath::Min(SelectionStartCoord.Y, SelectionCurrentCoord.Y);
	const int32 MaxY = FMath::Max(SelectionStartCoord.Y, SelectionCurrentCoord.Y);

	const FVector2D RectPos(
		PanOffset.X + (static_cast<float>(MinX) * CellPixelSize * EffectiveZoom),
		PanOffset.Y + (static_cast<float>(MinY) * CellPixelSize * EffectiveZoom));
	const FVector2D RectSize(
		static_cast<float>(MaxX - MinX + 1) * CellPixelSize * EffectiveZoom,
		static_cast<float>(MaxY - MinY + 1) * CellPixelSize * EffectiveZoom);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		SuperLayer + 1,
		AllottedGeometry.ToPaintGeometry(RectPos, RectSize),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		SelectionMarqueeColor);

	const float BorderThickness = 2.0f;
	const FLinearColor BorderColor = FLinearColor(SelectionMarqueeColor.R, SelectionMarqueeColor.G, SelectionMarqueeColor.B, 0.9f);
	const FVector2D TopSize(RectSize.X, BorderThickness);
	const FVector2D BottomPos(RectPos.X, RectPos.Y + RectSize.Y - BorderThickness);
	const FVector2D SideSize(BorderThickness, RectSize.Y);
	const FVector2D RightPos(RectPos.X + RectSize.X - BorderThickness, RectPos.Y);

	FSlateDrawElement::MakeBox(
		OutDrawElements, SuperLayer + 2, AllottedGeometry.ToPaintGeometry(RectPos, TopSize),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BorderColor);
	FSlateDrawElement::MakeBox(
		OutDrawElements, SuperLayer + 2, AllottedGeometry.ToPaintGeometry(BottomPos, TopSize),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BorderColor);
	FSlateDrawElement::MakeBox(
		OutDrawElements, SuperLayer + 2, AllottedGeometry.ToPaintGeometry(RectPos, SideSize),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BorderColor);
	FSlateDrawElement::MakeBox(
		OutDrawElements, SuperLayer + 2, AllottedGeometry.ToPaintGeometry(RightPos, SideSize),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BorderColor);

	return SuperLayer + 2;
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
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsSelecting)
	{
		EndSelection(true);
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bIsPanning)
	{
		bIsPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply ULevelDataGridEditorWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsSelecting)
	{
		if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			EndSelection(true);
			return FReply::Handled();
		}

		FIntPoint Coord = FIntPoint::ZeroValue;
		if (!TryGetCoordFromPointer(InGeometry, InMouseEvent.GetScreenSpacePosition(), Coord))
		{
			return FReply::Handled();
		}

		if (Coord != SelectionCurrentCoord)
		{
			SelectionCurrentCoord = Coord;
			bSelectionMoved = true;
			UpdateSelectionFromDragRect();
		}

		return FReply::Handled();
	}

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
	if (!IsViewCoordValid(Coord))
	{
		return;
	}
	const FModifierKeysState Modifiers = FSlateApplication::Get().GetModifierKeys();
	EGridSelectionOp Op = EGridSelectionOp::Replace;
	if (Modifiers.IsControlDown())
	{
		Op = EGridSelectionOp::Remove;
	}
	else if (Modifiers.IsShiftDown())
	{
		Op = EGridSelectionOp::Add;
	}

	// Start a selection gesture on mouse down; this supports both click-select and drag-select.
	BeginSelection(Coord, Op);
}

void ULevelDataGridEditorWidget::HandleCellHoveredWhilePressed(FIntPoint Coord)
{
	if (!bIsSelecting || !IsViewCoordValid(Coord))
	{
		return;
	}

	if (Coord != SelectionCurrentCoord)
	{
		SelectionCurrentCoord = Coord;
		bSelectionMoved = true;
		UpdateSelectionFromDragRect();
		UpdateSelectionMarqueeVisual();
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void ULevelDataGridEditorWidget::HandleCellMouseReleased()
{
	if (bIsSelecting)
	{
		EndSelection(true);
	}
}

void ULevelDataGridEditorWidget::InitializeFixedGridIfNeeded()
{
	if (bFixedGridInitialized)
	{
		return;
	}

	RebuildGrid();
	bFixedGridInitialized = true;
}

bool ULevelDataGridEditorWidget::ReloadMapDataWithoutRebuild(UPushBoxLevelData* InData, TArray<FCellDisplay>& InOutCells)
{
	if (!InData)
	{
		GridWidth = 0;
		GridHeight = 0;
		ResolvedCells.Reset();
		SelectedIndices.Reset();
		SelectionBaseIndices.Reset();
		ApplyValidRectVisibilityAndHitTest();
		UpdateContentBaseSize();
		ClampPanOffset();
		ApplyTransform();
		return false;
	}

	int32 NewGridWidth = 0;
	int32 NewGridHeight = 0;
	TSubclassOf<AGridCellBase> NewDefaultCellClass;
	TArray<TSubclassOf<AGridCellBase>> NewResolvedCells;
	BuildResolvedCellsFromLevelData(InData, NewGridWidth, NewGridHeight, NewDefaultCellClass, NewResolvedCells);

	if (NewGridWidth > FixedGridCapacity || NewGridHeight > FixedGridCapacity)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Load level failed: level size %dx%d exceeds fixed capacity %dx%d."),
			NewGridWidth, NewGridHeight, FixedGridCapacity, FixedGridCapacity);
		return false;
	}

	EditingLevelData = InData;
	if (TemporaryLevelData)
	{
		TemporaryLevelData->CopyFromLevelData(InData);
	}

	GridWidth = NewGridWidth;
	GridHeight = NewGridHeight;
	DefaultCellClass = NewDefaultCellClass;
	ResolvedCells = MoveTemp(NewResolvedCells);

	SyncCellDisplayList(InOutCells);
	TemporaryCellDisplayList = InOutCells;

	const TSet<int32> OldSelection = SelectedIndices;
	TSet<int32> NewSelection;
	for (const int32 Index : OldSelection)
	{
		if (Index >= 0 && Index < ResolvedCells.Num())
		{
			NewSelection.Add(Index);
		}
	}
	SelectedIndices = MoveTemp(NewSelection);
	SelectionBaseIndices = SelectedIndices;

	InitializeFixedGridIfNeeded();
	RefreshAllCells();
	ApplyValidRectVisibilityAndHitTest();
	RefreshSelectionVisuals(OldSelection);
	UpdateContentBaseSize();
	ResetView();
	SyncTemporaryLevelDataFromResolved();
	return true;
}

void ULevelDataGridEditorWidget::RebuildGrid()
{
	if (!GridPanel)
	{
		return;
	}

	GridPanel->ClearChildren();
	SpawnedCellWidgets.Reset();
	SpawnedCellWidgets.SetNum(GetPoolViewWidth() * GetPoolViewHeight());

	TSubclassOf<ULevelGridCellWidget> EffectiveCellWidgetClass = CellWidgetClass;
	if (!EffectiveCellWidgetClass)
	{
		EffectiveCellWidgetClass = ULevelGridCellWidget::StaticClass();
	}

	const int32 ViewWidth = GetPoolViewWidth();
	const int32 ViewHeight = GetPoolViewHeight();
	for (int32 ViewY = 0; ViewY < ViewHeight; ++ViewY)
	{
		for (int32 ViewX = 0; ViewX < ViewWidth; ++ViewX)
		{
			ULevelGridCellWidget* CellWidget = CreateWidget<ULevelGridCellWidget>(this, EffectiveCellWidgetClass);
			if (!CellWidget)
			{
				continue;
			}

			const FIntPoint ViewCoord(ViewX, ViewY);
			CellWidget->SetCellVisualSize(CellPixelSize);
			CellWidget->SetCellData(ViewCoord, DefaultCellClass, false);
			CellWidget->SetDisplayStyle(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f), nullptr);
			CellWidget->OnCellClicked.AddDynamic(this, &ULevelDataGridEditorWidget::HandleCellClicked);
			CellWidget->OnCellHoveredWhilePressed.AddDynamic(this, &ULevelDataGridEditorWidget::HandleCellHoveredWhilePressed);
			CellWidget->OnCellMouseReleased.AddDynamic(this, &ULevelDataGridEditorWidget::HandleCellMouseReleased);

			if (UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(CellWidget, ViewY, ViewX))
			{
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}

			const int32 PoolIndex = ViewCoordToPoolIndex(ViewCoord);
			if (SpawnedCellWidgets.IsValidIndex(PoolIndex))
			{
				SpawnedCellWidgets[PoolIndex] = CellWidget;
			}
		}
	}

	ApplyValidRectVisibilityAndHitTest();
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

			RefreshWidgetAtIndex(Index);
		}
	}
	ApplyValidRectVisibilityAndHitTest();
}

void ULevelDataGridEditorWidget::ApplyValidRectVisibilityAndHitTest()
{
	const int32 PoolWidth = GetPoolViewWidth();
	const int32 PoolHeight = GetPoolViewHeight();
	for (int32 ViewY = 0; ViewY < PoolHeight; ++ViewY)
	{
		for (int32 ViewX = 0; ViewX < PoolWidth; ++ViewX)
		{
			const FIntPoint ViewCoord(ViewX, ViewY);
			const int32 PoolIndex = ViewCoordToPoolIndex(ViewCoord);
			if (!SpawnedCellWidgets.IsValidIndex(PoolIndex) || !SpawnedCellWidgets[PoolIndex])
			{
				continue;
			}

			ULevelGridCellWidget* CellWidget = SpawnedCellWidgets[PoolIndex];
			const bool bValid = IsViewCoordValid(ViewCoord);
			CellWidget->SetVisibility(bValid ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
			CellWidget->SetIsEnabled(bValid);
			if (!bValid)
			{
				CellWidget->SetSelectedState(false);
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

bool ULevelDataGridEditorWidget::IsViewCoordValid(const FIntPoint& Coord) const
{
	return Coord.X >= 0 && Coord.Y >= 0 && Coord.X < GetViewWidth() && Coord.Y < GetViewHeight();
}

bool ULevelDataGridEditorWidget::IsViewCoordInPool(const FIntPoint& Coord) const
{
	return Coord.X >= 0 && Coord.Y >= 0 && Coord.X < GetPoolViewWidth() && Coord.Y < GetPoolViewHeight();
}

int32 ULevelDataGridEditorWidget::GetViewWidth() const
{
	return GridHeight > 0 ? GridHeight : 0;
}

int32 ULevelDataGridEditorWidget::GetViewHeight() const
{
	return GridWidth > 0 ? GridWidth : 0;
}

int32 ULevelDataGridEditorWidget::GetPoolViewWidth() const
{
	return FixedGridCapacity;
}

int32 ULevelDataGridEditorWidget::GetPoolViewHeight() const
{
	return FixedGridCapacity;
}

int32 ULevelDataGridEditorWidget::ViewCoordToPoolIndex(const FIntPoint& ViewCoord) const
{
	if (!IsViewCoordInPool(ViewCoord))
	{
		return INDEX_NONE;
	}

	return (ViewCoord.Y * GetPoolViewWidth()) + ViewCoord.X;
}

FIntPoint ULevelDataGridEditorWidget::ViewToDataCoord(const FIntPoint& ViewCoord) const
{
	if (GridWidth <= 0 || GridHeight <= 0)
	{
		return FIntPoint(-1, -1);
	}

	// 90-degree CCW view rotation.
	// view(x, y) -> data(W - 1 - y, x)
	return FIntPoint(GridWidth - 1 - ViewCoord.Y, ViewCoord.X);
}

FIntPoint ULevelDataGridEditorWidget::DataToViewCoord(const FIntPoint& DataCoord) const
{
	if (GridWidth <= 0 || GridHeight <= 0)
	{
		return FIntPoint(-1, -1);
	}

	// Inverse mapping of ViewToDataCoord.
	// data(x, y) -> view(y, W - 1 - x)
	return FIntPoint(DataCoord.Y, GridWidth - 1 - DataCoord.X);
}

void ULevelDataGridEditorWidget::UpdateContentBaseSize()
{
	ContentBaseSize = FVector2D(
		FMath::Max(0, GetViewWidth()) * CellPixelSize,
		FMath::Max(0, GetViewHeight()) * CellPixelSize);
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

void ULevelDataGridEditorWidget::SyncTemporaryLevelDataFromResolved()
{
	if (!TemporaryLevelData)
	{
		return;
	}

	if (GridWidth <= 0 || GridHeight <= 0)
	{
		return;
	}

	TemporaryLevelData->GridWidth = GridWidth;
	TemporaryLevelData->GridHeight = GridHeight;
	TemporaryLevelData->DefaultCellClass = DefaultCellClass;
	TemporaryLevelData->InitializeGrid(GridWidth, GridHeight, DefaultCellClass);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const int32 Index = ToIndex(FIntPoint(X, Y));
			const TSubclassOf<AGridCellBase> CellClass = ResolvedCells.IsValidIndex(Index) ? ResolvedCells[Index] : nullptr;
			TemporaryLevelData->SetCellAt(FIntPoint(X, Y), CellClass);
		}
	}
}

void ULevelDataGridEditorWidget::BuildResolvedCellsFromLevelData(
	const UPushBoxLevelData* SourceData,
	int32& OutGridWidth,
	int32& OutGridHeight,
	TSubclassOf<AGridCellBase>& OutDefaultCellClass,
	TArray<TSubclassOf<AGridCellBase>>& OutResolvedCells) const
{
	OutGridWidth = FMath::Max(0, SourceData ? SourceData->GridWidth : 0);
	OutGridHeight = FMath::Max(0, SourceData ? SourceData->GridHeight : 0);
	OutDefaultCellClass = SourceData ? SourceData->DefaultCellClass : nullptr;

	OutResolvedCells.Reset();
	OutResolvedCells.SetNum(OutGridWidth * OutGridHeight);
	for (int32 Index = 0; Index < OutResolvedCells.Num(); ++Index)
	{
		OutResolvedCells[Index] = OutDefaultCellClass;
	}

	if (!SourceData)
	{
		return;
	}

	for (int32 Y = 0; Y < OutGridHeight; ++Y)
	{
		for (int32 X = 0; X < OutGridWidth; ++X)
		{
			const FIntPoint Coord(X, Y);
			const int32 Index = Coord.Y * OutGridWidth + Coord.X;
			if (!OutResolvedCells.IsValidIndex(Index))
			{
				continue;
			}

			TSubclassOf<AGridCellBase> CellClass = SourceData->GetCellAt(Coord);
			if (!CellClass)
			{
				CellClass = OutDefaultCellClass;
			}
			OutResolvedCells[Index] = CellClass;
		}
	}
}

void ULevelDataGridEditorWidget::RefreshWidgetAtIndex(int32 Index)
{
	if (!ResolvedCells.IsValidIndex(Index))
	{
		return;
	}

	const int32 X = Index % GridWidth;
	const int32 Y = Index / GridWidth;
	const FIntPoint DataCoord(X, Y);
	const FIntPoint ViewCoord = DataToViewCoord(DataCoord);
	const int32 PoolIndex = ViewCoordToPoolIndex(ViewCoord);
	if (!SpawnedCellWidgets.IsValidIndex(PoolIndex) || !SpawnedCellWidgets[PoolIndex])
	{
		return;
	}

	const TSubclassOf<AGridCellBase> CellClass = ResolvedCells.IsValidIndex(Index) ? ResolvedCells[Index] : DefaultCellClass;

	SpawnedCellWidgets[PoolIndex]->SetCellData(ViewCoord, CellClass, true);
	if (const FCellDisplay* Display = CellDisplayLookup.Find(CellClass))
	{
		SpawnedCellWidgets[PoolIndex]->SetDisplayStyle(Display->BGColor, Display->Icon);
	}
	else
	{
		SpawnedCellWidgets[PoolIndex]->SetDisplayStyle(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f), nullptr);
	}
	SpawnedCellWidgets[PoolIndex]->SetSelectedState(SelectedIndices.Contains(Index));
}

bool ULevelDataGridEditorWidget::IsCellDisplayEqual(const FCellDisplay& A, const FCellDisplay& B) const
{
	return A.CellType == B.CellType && A.Icon == B.Icon && A.BGColor.Equals(B.BGColor, KINDA_SMALL_NUMBER);
}

bool ULevelDataGridEditorWidget::TryGetCoordFromPointer(const FGeometry& InGeometry, const FVector2D& InScreenPosition, FIntPoint& OutCoord) const
{
	const FVector2D Local = InGeometry.AbsoluteToLocal(InScreenPosition);
	const FVector2D ContentLocal = (Local - PanOffset) / FMath::Max(CurrentZoom, KINDA_SMALL_NUMBER);

	const int32 ViewX = FMath::FloorToInt(ContentLocal.X / FMath::Max(CellPixelSize, 1.0f));
	const int32 ViewY = FMath::FloorToInt(ContentLocal.Y / FMath::Max(CellPixelSize, 1.0f));
	const FIntPoint ViewCoord(ViewX, ViewY);
	if (!IsViewCoordInPool(ViewCoord))
	{
		return false;
	}

	if (!IsViewCoordValid(ViewCoord))
	{
		return false;
	}

	OutCoord = ViewCoord;
	return true;
}

int32 ULevelDataGridEditorWidget::CoordToIndex(const FIntPoint& Coord) const
{
	if (!IsCoordValid(Coord))
	{
		return INDEX_NONE;
	}
	return ToIndex(Coord);
}

void ULevelDataGridEditorWidget::RefreshSelectionVisuals(const TSet<int32>& OldSelection)
{
	TSet<int32> Dirty = OldSelection;
	Dirty.Append(SelectedIndices);
	for (const int32 Index : Dirty)
	{
		RefreshWidgetAtIndex(Index);
	}
}

void ULevelDataGridEditorWidget::UpdateSelectionFromDragRect()
{
	TSet<int32> RectSet;
	BuildRectSelectionSet(SelectionStartCoord, SelectionCurrentCoord, RectSet);

	const TSet<int32> OldSelection = SelectedIndices;
	if (CurrentSelectionOp == EGridSelectionOp::Replace)
	{
		SelectedIndices = MoveTemp(RectSet);
	}
	else if (CurrentSelectionOp == EGridSelectionOp::Add)
	{
		SelectedIndices = SelectionBaseIndices;
		SelectedIndices.Append(RectSet);
	}
	else
	{
		SelectedIndices = SelectionBaseIndices;
		for (const int32 Index : RectSet)
		{
			SelectedIndices.Remove(Index);
		}
	}

	RefreshSelectionVisuals(OldSelection);
}

void ULevelDataGridEditorWidget::BuildRectSelectionSet(const FIntPoint& A, const FIntPoint& B, TSet<int32>& OutSet) const
{
	OutSet.Reset();
	const int32 MinX = FMath::Min(A.X, B.X);
	const int32 MaxX = FMath::Max(A.X, B.X);
	const int32 MinY = FMath::Min(A.Y, B.Y);
	const int32 MaxY = FMath::Max(A.Y, B.Y);

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const FIntPoint ViewCoord(X, Y);
			if (!IsViewCoordValid(ViewCoord))
			{
				continue;
			}

			const FIntPoint DataCoord = ViewToDataCoord(ViewCoord);
			const int32 Index = CoordToIndex(DataCoord);
			if (Index != INDEX_NONE)
			{
				OutSet.Add(Index);
			}
		}
	}
}

EGridSelectionOp ULevelDataGridEditorWidget::ResolveSelectionOp(const FPointerEvent& InMouseEvent) const
{
	if (InMouseEvent.IsControlDown())
	{
		return EGridSelectionOp::Remove;
	}
	if (InMouseEvent.IsShiftDown())
	{
		return EGridSelectionOp::Add;
	}
	return EGridSelectionOp::Replace;
}

void ULevelDataGridEditorWidget::BeginSelection(const FIntPoint& StartCoord, EGridSelectionOp Op)
{
	bIsSelecting = true;
	bSelectionMoved = false;
	SelectionStartCoord = StartCoord;
	SelectionCurrentCoord = StartCoord;
	CurrentSelectionOp = Op;
	SelectionBaseIndices = SelectedIndices;
	UpdateSelectionFromDragRect();
	UpdateSelectionMarqueeVisual();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void ULevelDataGridEditorWidget::EndSelection(bool bCommitSelection)
{
	if (!bCommitSelection)
	{
		SelectedIndices = SelectionBaseIndices;
		RefreshSelectionVisuals(SelectionBaseIndices);
	}

	bIsSelecting = false;
	bSelectionMoved = false;
	SelectionBaseIndices = SelectedIndices;
	UpdateSelectionMarqueeVisual();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void ULevelDataGridEditorWidget::EnsureSelectionMarqueeWidget()
{
	if (SelectionMarqueeWidget || !GridCanvas || !WidgetTree)
	{
		return;
	}

	SelectionMarqueeWidget = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SelectionMarquee"));
	if (!SelectionMarqueeWidget)
	{
		return;
	}

	SelectionMarqueeWidget->SetBrushColor(SelectionMarqueeColor);
	SelectionMarqueeWidget->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* CanvasSlot = GridCanvas->AddChildToCanvas(SelectionMarqueeWidget))
	{
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetZOrder(1000);
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
		CanvasSlot->SetSize(FVector2D::ZeroVector);
	}
}

void ULevelDataGridEditorWidget::UpdateSelectionMarqueeVisual()
{
	if (!SelectionMarqueeWidget)
	{
		return;
	}

	SelectionMarqueeWidget->SetBrushColor(SelectionMarqueeColor);
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SelectionMarqueeWidget->Slot);
	if (!CanvasSlot)
	{
		return;
	}

	if (!bIsSelecting || (!bSelectionMoved && SelectionStartCoord == SelectionCurrentCoord))
	{
		SelectionMarqueeWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const int32 MinX = FMath::Min(SelectionStartCoord.X, SelectionCurrentCoord.X);
	const int32 MaxX = FMath::Max(SelectionStartCoord.X, SelectionCurrentCoord.X);
	const int32 MinY = FMath::Min(SelectionStartCoord.Y, SelectionCurrentCoord.Y);
	const int32 MaxY = FMath::Max(SelectionStartCoord.Y, SelectionCurrentCoord.Y);

	const FVector2D Pos(static_cast<float>(MinX) * CellPixelSize, static_cast<float>(MinY) * CellPixelSize);
	const FVector2D Size(
		static_cast<float>(MaxX - MinX + 1) * CellPixelSize,
		static_cast<float>(MaxY - MinY + 1) * CellPixelSize);

	CanvasSlot->SetPosition(Pos);
	CanvasSlot->SetSize(Size);
	SelectionMarqueeWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
