// Copyright Epic Games, Inc. All Rights Reserved.

#include "CellDisplayCustomization.h"

#include "DetailWidgetRow.h"
#include "Editor.h"
#include "GridCellBase.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "PushBoxEditorBridgeSubsystem.h"
#include "SaveDataTypes.h"
#include "Engine/Texture2D.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "CellDisplayCustomization"

TSharedRef<IPropertyTypeCustomization> FCellDisplayCustomization::MakeInstance()
{
	return MakeShared<FCellDisplayCustomization>();
}

void FCellDisplayCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	RootHandle = StructPropertyHandle;

	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
		if (!ChildHandle.IsValid() || !ChildHandle->IsValidHandle() || !ChildHandle->GetProperty())
		{
			continue;
		}

		const FName PropertyName = ChildHandle->GetProperty()->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(FCellDisplay, CellType))
		{
			CellTypeHandle = ChildHandle;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FCellDisplay, BGColor))
		{
			BGColorHandle = ChildHandle;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FCellDisplay, Icon))
		{
			IconHandle = ChildHandle;
		}
	}

	HeaderRow
	.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()]
	.ValueContent()
	.MinDesiredWidth(320.0f)
	.MaxDesiredWidth(1000.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 0.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(LOCTEXT("ApplyCellDisplayInline", "Apply"))
			.OnClicked(FOnClicked::CreateSP(this, &FCellDisplayCustomization::HandleApplyClicked))
		]
	];
}

void FCellDisplayCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	RootHandle = StructPropertyHandle;

	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
		if (!ChildHandle.IsValid() || !ChildHandle->IsValidHandle())
		{
			continue;
		}

		StructBuilder.AddProperty(ChildHandle.ToSharedRef());
	}
}

FReply FCellDisplayCustomization::HandleApplyClicked() const
{
	if (!GEditor)
	{
		return FReply::Handled();
	}

	UPushBoxEditorBridgeSubsystem* Bridge = GEditor->GetEditorSubsystem<UPushBoxEditorBridgeSubsystem>();
	if (!Bridge)
	{
		return FReply::Handled();
	}

	FCellDisplay Payload;

	if (CellTypeHandle.IsValid())
	{
		UObject* CellTypeObject = nullptr;
		if (CellTypeHandle->GetValue(CellTypeObject) == FPropertyAccess::Success)
		{
			Payload.CellType = Cast<UClass>(CellTypeObject);
		}
	}

	if (BGColorHandle.IsValid())
	{
		TArray<void*> RawData;
		BGColorHandle->AccessRawData(RawData);
		if (RawData.Num() > 0 && RawData[0] != nullptr)
		{
			Payload.BGColor = *static_cast<FLinearColor*>(RawData[0]);
		}
	}

	if (IconHandle.IsValid())
	{
		UObject* IconObject = nullptr;
		if (IconHandle->GetValue(IconObject) == FPropertyAccess::Success)
		{
			Payload.Icon = Cast<UTexture2D>(IconObject);
		}
	}

	const FString PropertyPath = RootHandle.IsValid() ? RootHandle->GeneratePathToProperty() : FString();
	Bridge->BroadcastApplyCellDisplayRequested(Payload, PropertyPath);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
