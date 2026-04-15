// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidgetComponents.h"
#include "NumericEditorUtilityEditableTextBox.generated.h"

UCLASS(BlueprintType, meta = (DisplayName = "Numeric Editor Utility Editable Text Box"))
class PUSHBOXEDITOR_API UNumericEditorUtilityEditableTextBox : public UEditorUtilityEditableTextBox
{
	GENERATED_BODY()

public:
	UNumericEditorUtilityEditableTextBox(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numeric")
	bool bAllowEmpty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numeric")
	int32 DefaultValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numeric")
	bool bUseMinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numeric", meta = (EditCondition = "bUseMinValue"))
	int32 MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numeric")
	bool bUseMaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numeric", meta = (EditCondition = "bUseMaxValue"))
	int32 MaxValue;

	UFUNCTION(BlueprintPure, Category = "Numeric")
	int32 GetIntValue() const;

	UFUNCTION(BlueprintCallable, Category = "Numeric")
	void SetIntValue(int32 InValue);

protected:
	virtual void HandleOnTextChanged(const FText& InText) override;
	virtual void HandleOnTextCommitted(const FText& InText, ETextCommit::Type CommitMethod) override;

private:
	UFUNCTION()
	void HandleSelfTextChanged(const FText& InText);

	UFUNCTION()
	void HandleSelfTextCommitted(const FText& InText, ETextCommit::Type CommitMethod);

	bool bInternalUpdating;

	FString FilterDigits(const FString& Input) const;
	int32 NormalizeValue(int32 InValue) const;
};
