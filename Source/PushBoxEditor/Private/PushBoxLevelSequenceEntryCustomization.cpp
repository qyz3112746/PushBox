// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelSequenceEntryCustomization.h"

#include "DetailWidgetRow.h"
#include "Editor.h"
#include "IDetailChildrenBuilder.h"
#include "LevelProcessController.h"
#include "PropertyHandle.h"
#include "PushBoxLevelData.h"
#include "PushBoxLevelPreviewSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<IPropertyTypeCustomization> FPushBoxLevelSequenceEntryCustomization::MakeInstance()
{
	return MakeShared<FPushBoxLevelSequenceEntryCustomization>();
}

void FPushBoxLevelSequenceEntryCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	RootHandle = StructPropertyHandle;
	LevelDataHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPushBoxLevelSequenceEntry, LevelData));

	HeaderRow
	.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()]
	.ValueContent()
	.MinDesiredWidth(320.0f)
	.MaxDesiredWidth(1400.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			LevelDataHandle.IsValid()
				? LevelDataHandle->CreatePropertyValueWidget()
				: StructPropertyHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 0.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("PushBoxLevelSequenceEntryCustomization", "Preview", "Preview"))
			.OnClicked(FOnClicked::CreateSP(this, &FPushBoxLevelSequenceEntryCustomization::HandlePreviewClicked))
		]
	];
}

void FPushBoxLevelSequenceEntryCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

FReply FPushBoxLevelSequenceEntryCustomization::HandlePreviewClicked() const
{
	if (!GEditor || !LevelDataHandle.IsValid())
	{
		return FReply::Handled();
	}

	UObject* LevelDataObject = nullptr;
	if (LevelDataHandle->GetValue(LevelDataObject) != FPropertyAccess::Success)
	{
		return FReply::Handled();
	}

	UPushBoxLevelData* LevelData = Cast<UPushBoxLevelData>(LevelDataObject);
	ALevelProcessController* ProcessController = ResolveOwnerProcessController();
	const int32 SequenceIndex = ResolveSequenceIndex();
	if (!ProcessController || !LevelData || SequenceIndex == INDEX_NONE)
	{
		return FReply::Handled();
	}

	if (UPushBoxLevelPreviewSubsystem* PreviewSubsystem = GEditor->GetEditorSubsystem<UPushBoxLevelPreviewSubsystem>())
	{
		PreviewSubsystem->StartPreview(ProcessController, LevelData, SequenceIndex);
	}

	return FReply::Handled();
}

ALevelProcessController* FPushBoxLevelSequenceEntryCustomization::ResolveOwnerProcessController() const
{
	if (!RootHandle.IsValid())
	{
		return nullptr;
	}

	TArray<UObject*> OuterObjects;
	RootHandle->GetOuterObjects(OuterObjects);
	for (UObject* Outer : OuterObjects)
	{
		if (ALevelProcessController* Controller = Cast<ALevelProcessController>(Outer))
		{
			return Controller;
		}
	}

	return nullptr;
}

int32 FPushBoxLevelSequenceEntryCustomization::ResolveSequenceIndex() const
{
	if (!RootHandle.IsValid())
	{
		return INDEX_NONE;
	}

	const FString Path = RootHandle->GeneratePathToProperty();
	int32 OpenBracketIndex = INDEX_NONE;
	if (!Path.FindLastChar(TEXT('['), OpenBracketIndex))
	{
		return INDEX_NONE;
	}

	int32 CloseBracketIndex = INDEX_NONE;
	if (!Path.FindChar(TEXT(']'), CloseBracketIndex) || CloseBracketIndex <= OpenBracketIndex)
	{
		return INDEX_NONE;
	}

	const FString IndexString = Path.Mid(OpenBracketIndex + 1, CloseBracketIndex - OpenBracketIndex - 1);
	return FCString::Atoi(*IndexString);
}

