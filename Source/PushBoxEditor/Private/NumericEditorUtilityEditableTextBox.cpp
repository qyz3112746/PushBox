// Copyright Epic Games, Inc. All Rights Reserved.

#include "NumericEditorUtilityEditableTextBox.h"

UNumericEditorUtilityEditableTextBox::UNumericEditorUtilityEditableTextBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bAllowEmpty(true)
	, DefaultValue(0)
	, bUseMinValue(false)
	, MinValue(0)
	, bUseMaxValue(false)
	, MaxValue(1000000)
	, bInternalUpdating(false)
{
	OnTextChanged.AddUniqueDynamic(this, &UNumericEditorUtilityEditableTextBox::HandleSelfTextChanged);
	OnTextCommitted.AddUniqueDynamic(this, &UNumericEditorUtilityEditableTextBox::HandleSelfTextCommitted);
}

FString UNumericEditorUtilityEditableTextBox::FilterDigits(const FString& Input) const
{
	FString Result;
	Result.Reserve(Input.Len());

	for (const TCHAR Ch : Input)
	{
		if (FChar::IsDigit(Ch))
		{
			Result.AppendChar(Ch);
		}
	}

	return Result;
}

int32 UNumericEditorUtilityEditableTextBox::NormalizeValue(int32 InValue) const
{
	int32 Result = InValue;
	if (bUseMinValue)
	{
		Result = FMath::Max(Result, MinValue);
	}

	if (bUseMaxValue)
	{
		Result = FMath::Min(Result, MaxValue);
	}

	return Result;
}

int32 UNumericEditorUtilityEditableTextBox::GetIntValue() const
{
	const FString Digits = FilterDigits(GetText().ToString());
	if (Digits.IsEmpty())
	{
		return NormalizeValue(DefaultValue);
	}

	const int64 Parsed = FCString::Atoi64(*Digits);
	return NormalizeValue(static_cast<int32>(FMath::Clamp<int64>(Parsed, TNumericLimits<int32>::Min(), TNumericLimits<int32>::Max())));
}

void UNumericEditorUtilityEditableTextBox::SetIntValue(int32 InValue)
{
	const int32 Normalized = NormalizeValue(InValue);
	bInternalUpdating = true;
	SetText(FText::FromString(FString::FromInt(Normalized)));
	bInternalUpdating = false;
}

void UNumericEditorUtilityEditableTextBox::HandleOnTextChanged(const FText& InText)
{
	Super::HandleOnTextChanged(InText);
}

void UNumericEditorUtilityEditableTextBox::HandleOnTextCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
	Super::HandleOnTextCommitted(InText, CommitMethod);
}

void UNumericEditorUtilityEditableTextBox::HandleSelfTextChanged(const FText& InText)
{
	if (bInternalUpdating)
	{
		return;
	}

	const FString Original = InText.ToString();
	FString Filtered = FilterDigits(Original);
	if (Filtered.IsEmpty())
	{
		if (!bAllowEmpty)
		{
			Filtered = FString::FromInt(NormalizeValue(DefaultValue));
		}

		if (Filtered == Original)
		{
			return;
		}

		bInternalUpdating = true;
		SetText(FText::FromString(Filtered));
		bInternalUpdating = false;
		return;
	}

	const int64 Parsed = FCString::Atoi64(*Filtered);
	const int32 Normalized = NormalizeValue(static_cast<int32>(FMath::Clamp<int64>(Parsed, TNumericLimits<int32>::Min(), TNumericLimits<int32>::Max())));
	const FString NormalizedString = FString::FromInt(Normalized);

	if (NormalizedString == Original)
	{
		return;
	}

	bInternalUpdating = true;
	SetText(FText::FromString(NormalizedString));
	bInternalUpdating = false;
}

void UNumericEditorUtilityEditableTextBox::HandleSelfTextCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
	if (bInternalUpdating)
	{
		return;
	}

	FString Digits = FilterDigits(InText.ToString());
	if (!bAllowEmpty && Digits.IsEmpty())
	{
		Digits = FString::FromInt(DefaultValue);
	}

	if (Digits.IsEmpty())
	{
		bInternalUpdating = true;
		SetText(FText::GetEmpty());
		bInternalUpdating = false;
		return;
	}

	const int64 Parsed = FCString::Atoi64(*Digits);
	const int32 Normalized = NormalizeValue(static_cast<int32>(FMath::Clamp<int64>(Parsed, TNumericLimits<int32>::Min(), TNumericLimits<int32>::Max())));
	const FString NormalizedString = FString::FromInt(Normalized);

	if (NormalizedString != InText.ToString())
	{
		bInternalUpdating = true;
		SetText(FText::FromString(NormalizedString));
		bInternalUpdating = false;
	}
}
