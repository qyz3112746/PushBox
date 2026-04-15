// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PushBoxLevelData.h"
#include "LevelDataGridEditorWidget.generated.h"

class UCanvasPanel;
class UUniformGridPanel;
class ULevelGridCellWidget;
class AGridCellBase;

UCLASS(BlueprintType)
class PUSHBOXEDITOR_API ULevelDataGridEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	bool LoadFromLevelData(UPushBoxLevelData* InData);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	bool WriteBackToLevelData();

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Data")
	void SetMapSize(int32 InWidth, int32 InHeight);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Edit")
	void SetActiveBrushCellClass(TSubclassOf<AGridCellBase> InClass);

	UFUNCTION(BlueprintCallable, Category = "LevelEditor|Edit")
	void ClearToDefault();

	UFUNCTION(BlueprintPure, Category = "LevelEditor|Data")
	UPushBoxLevelData* GetEditingLevelData() const { return EditingLevelData; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandleCellClicked(FIntPoint Coord);

	void RebuildGrid();
	void RefreshAllCells();
	void ApplyTransform();
	void ClampPanOffset();
	void ResetView();
	float ComputeFitZoom() const;
	void AutoFitView(bool bResetPan);
	FVector2D GetViewportSize() const;
	int32 ToIndex(const FIntPoint& Coord) const;
	bool IsCoordValid(const FIntPoint& Coord) const;
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

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelGridCellWidget>> SpawnedCellWidgets;

	UPROPERTY(Transient)
	TArray<TSubclassOf<AGridCellBase>> ResolvedCells;

	int32 GridWidth = 1;
	int32 GridHeight = 1;
	bool bIsPanning = false;
	FVector2D LastMouseScreenPos = FVector2D::ZeroVector;
	FVector2D LastViewportSize = FVector2D::ZeroVector;
	bool bHasUserAdjustedView = false;
};
