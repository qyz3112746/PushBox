// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PushBoxLevelData.h"
#include "SaveDataTypes.h"
#include "LevelDataGridEditorWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UUniformGridPanel;
class ULevelGridCellWidget;
class AGridCellBase;
class UPushBoxLevelData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelDataAssetSaved, UPushBoxLevelData*, SavedLevelDataAsset);

UENUM()
enum class EGridSelectionOp : uint8
{
	Replace,
	Add,
	Remove
};

UCLASS(BlueprintType)
class PUSHBOXEDITOR_API ULevelDataGridEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	bool LoadFromLevelDataWithCellDisplay(UPushBoxLevelData* InData, UPARAM(ref) TArray<FCellDisplay>& InOutCells);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	bool WriteBackToLevelData();

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	bool RefreshLevelDisplayWithCellDisplay(UPushBoxLevelData* InData, UPARAM(ref) TArray<FCellDisplay>& InOutCells);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	void SetMapSize(int32 InWidth, int32 InHeight);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Edit")
	bool ApplyCellDisplayToSelection(const FCellDisplay& InDisplay);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Edit")
	void ClearSelection();

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Edit")
	void SetActiveBrushCellClass(TSubclassOf<AGridCellBase> InClass);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Edit")
	void ClearToDefault();

	UFUNCTION(BlueprintPure, Category = "LevelEditor|Data")
	UPushBoxLevelData* GetEditingLevelData() const { return EditingLevelData; }

	UFUNCTION(BlueprintPure, Category = "LevelEditor|Data")
	UPushBoxLevelData* GetTemporaryLevelData() const { return TemporaryLevelData; }

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	bool SaveTemporaryLevelDataAsAssetWithDialog();

	UPROPERTY(BlueprintAssignable, Category = "LevelEditor|Data")
	FOnLevelDataAssetSaved OnLevelDataAssetSaved;

protected:
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandleCellClicked(FIntPoint Coord);

	UFUNCTION()
	void HandleCellHoveredWhilePressed(FIntPoint Coord);

	UFUNCTION()
	void HandleCellMouseReleased();

	void RebuildGrid();
	void RefreshAllCells();
	void ApplyTransform();
	void ClampPanOffset();
	void ResetView();
	float ComputeFitZoom() const;
	void AutoFitView(bool bResetPan);
	void SyncCellDisplayList(UPARAM(ref) TArray<FCellDisplay>& InOutCells);
	FCellDisplay BuildRandomCellDisplay(TSubclassOf<AGridCellBase> CellType) const;
	void RebuildDisplayLookupFromArray(const TArray<FCellDisplay>& InCells);
	void BuildResolvedCellsFromLevelData(
		const UPushBoxLevelData* SourceData,
		int32& OutGridWidth,
		int32& OutGridHeight,
		TSubclassOf<AGridCellBase>& OutDefaultCellClass,
		TArray<TSubclassOf<AGridCellBase>>& OutResolvedCells) const;
	void RefreshWidgetAtIndex(int32 Index);
	bool IsCellDisplayEqual(const FCellDisplay& A, const FCellDisplay& B) const;
	void SyncTemporaryLevelDataFromResolved();
	bool TryGetCoordFromPointer(const FGeometry& InGeometry, const FVector2D& InScreenPosition, FIntPoint& OutCoord) const;
	int32 CoordToIndex(const FIntPoint& Coord) const;
	FIntPoint ViewToDataCoord(const FIntPoint& ViewCoord) const;
	FIntPoint DataToViewCoord(const FIntPoint& DataCoord) const;
	int32 GetViewWidth() const;
	int32 GetViewHeight() const;
	void RefreshSelectionVisuals(const TSet<int32>& OldSelection);
	void UpdateSelectionFromDragRect();
	void BuildRectSelectionSet(const FIntPoint& A, const FIntPoint& B, TSet<int32>& OutSet) const;
	EGridSelectionOp ResolveSelectionOp(const FPointerEvent& InMouseEvent) const;
	void BeginSelection(const FIntPoint& StartCoord, EGridSelectionOp Op);
	void EndSelection(bool bCommitSelection);
	void EnsureSelectionMarqueeWidget();
	void UpdateSelectionMarqueeVisual();
	FVector2D GetViewportSize() const;
	int32 ToIndex(const FIntPoint& Coord) const;
	bool IsCoordValid(const FIntPoint& Coord) const;
	bool IsViewCoordValid(const FIntPoint& Coord) const;
	void UpdateContentBaseSize();

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LevelEditor")
	TObjectPtr<UCanvasPanel> GridCanvas;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LevelEditor")
	TObjectPtr<UUniformGridPanel> GridPanel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor")
	TSubclassOf<ULevelGridCellWidget> CellWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|Data")
	TObjectPtr<UPushBoxLevelData> EditingLevelData;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "LevelEditor|Data")
	TObjectPtr<UPushBoxLevelData> TemporaryLevelData;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "LevelEditor|Data")
	TArray<FCellDisplay> TemporaryCellDisplayList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|Data")
	TSubclassOf<AGridCellBase> DefaultCellClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|Edit")
	TArray<TSubclassOf<AGridCellBase>> PaletteCells;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|Edit")
	TSubclassOf<AGridCellBase> ActiveBrushCellClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|View", meta = (ClampMin = "4.0"))
	float CellPixelSize = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|View", meta = (ClampMin = "0.01"))
	float ZoomMin = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|View", meta = (ClampMin = "0.01"))
	float ZoomMax = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|View", meta = (ClampMin = "1.01"))
	float ZoomStep = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|View")
	bool bClampPan = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|View")
	bool bAutoFitOnReset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|View", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float FitPaddingPercent = 0.05f;

	UPROPERTY(BlueprintReadOnly, Category = "LevelEditor|View")
	float CurrentZoom = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "LevelEditor|View")
	FVector2D PanOffset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "LevelEditor|View")
	FVector2D ContentBaseSize = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|Selection")
	FLinearColor SelectionMarqueeColor = FLinearColor(0.25f, 0.55f, 1.0f, 0.18f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|Selection")
	FLinearColor SelectedCellOutlineColor = FLinearColor(1.0f, 0.95f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelEditor|Selection", meta = (ClampMin = "1.0", ClampMax = "8.0"))
	float SelectedCellOutlineThickness = 2.0f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelGridCellWidget>> SpawnedCellWidgets;

	UPROPERTY(Transient)
	TArray<TSubclassOf<AGridCellBase>> ResolvedCells;

	UPROPERTY(Transient)
	TMap<TSubclassOf<AGridCellBase>, FCellDisplay> CellDisplayLookup;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SelectionMarqueeWidget;

	int32 GridWidth = 1;
	int32 GridHeight = 1;
	bool bIsPanning = false;
	bool bIsSelecting = false;
	bool bSelectionMoved = false;
	FVector2D LastMouseScreenPos = FVector2D::ZeroVector;
	FVector2D LastViewportSize = FVector2D::ZeroVector;
	bool bHasUserAdjustedView = false;
	FIntPoint SelectionStartCoord = FIntPoint::ZeroValue;
	FIntPoint SelectionCurrentCoord = FIntPoint::ZeroValue;
	EGridSelectionOp CurrentSelectionOp = EGridSelectionOp::Replace;
	TSet<int32> SelectedIndices;
	TSet<int32> SelectionBaseIndices;
};
