// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LevelGridCellWidget.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;
class UImage;
class UTexture2D;
class AGridCellBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelGridCellClicked, FIntPoint, Coord);

UCLASS(BlueprintType)
class PUSHBOXEDITOR_API ULevelGridCellWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Grid")
	FOnLevelGridCellClicked OnCellClicked;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetCellData(const FIntPoint& InCoord, TSubclassOf<AGridCellBase> InCellClass, bool bInIsValidCoord);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetDisplayStyle(const FLinearColor& InColor, UTexture2D* InIcon);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetCellVisualSize(float InSize);

	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint GetCoord() const { return Coord; }

	UFUNCTION(BlueprintPure, Category = "Grid")
	TSubclassOf<AGridCellBase> GetCellClass() const { return CellClass; }

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsValidCoord() const { return bIsValidCoord; }

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Grid")
	void BP_OnCellVisualRefresh();

	void RefreshVisuals();

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Grid")
	TObjectPtr<USizeBox> CellSizeBox;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UBorder> CellBorder;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UTextBlock> CellLabel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UImage> CellIconImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Style")
	FLinearColor ValidCellColor = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Style")
	FLinearColor InvalidCellColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Style")
	bool bShowClassNameInLabel = false;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	TSubclassOf<AGridCellBase> CellClass;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	bool bIsValidCoord = true;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FLinearColor DisplayColor = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UTexture2D> DisplayIcon = nullptr;
};
