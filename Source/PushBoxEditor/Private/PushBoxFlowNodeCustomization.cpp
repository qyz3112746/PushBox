// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxFlowNodeCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "LevelProcessController.h"
#include "PropertyHandle.h"
#include "PushBoxFlowDataAsset.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PushBoxFlowNodeCustomization"

TSharedRef<IPropertyTypeCustomization> FPushBoxFlowNodeCustomization::MakeInstance()
{
	return MakeShared<FPushBoxFlowNodeCustomization>();
}

void FPushBoxFlowNodeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	RootHandle = StructPropertyHandle;
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	HeaderRow
	.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()]
	.ValueContent()
	.MinDesiredWidth(320.0f)
	.MaxDesiredWidth(1200.0f)
	[
		StructPropertyHandle->CreatePropertyValueWidget()
	];
}

void FPushBoxFlowNodeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	RootHandle = StructPropertyHandle;
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

	NodeIdHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPushBoxFlowNode, NodeId));
	ProcessControllerHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPushBoxFlowNode, ProcessController));

	if (NodeIdHandle.IsValid() && NodeIdHandle->IsValidHandle())
	{
		StructBuilder.AddProperty(NodeIdHandle.ToSharedRef());
	}

	if (ProcessControllerHandle.IsValid() && ProcessControllerHandle->IsValidHandle())
	{
		ProcessControllerHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPushBoxFlowNodeCustomization::HandleProcessControllerChanged));
		StructBuilder.AddProperty(ProcessControllerHandle.ToSharedRef());
	}

	ALevelProcessController* ProcessController = ResolveProcessController();
	if (!ProcessController)
	{
		StructBuilder.AddCustomRow(LOCTEXT("FlowNodeControllerHint", "Process Controller Hint"))
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FlowNodeControllerInvalid", "Please select a loaded ProcessController actor in the current level to edit LevelSequence."))
			.AutoWrapText(true)
		];
		return;
	}

	TArray<UObject*> ExternalObjects;
	ExternalObjects.Add(ProcessController);
	StructBuilder.AddExternalObjectProperty(ExternalObjects, FName(TEXT("CellSize")));
	StructBuilder.AddExternalObjectProperty(ExternalObjects, GET_MEMBER_NAME_CHECKED(ALevelProcessController, LevelSequence));
}

void FPushBoxFlowNodeCustomization::HandleProcessControllerChanged()
{
	if (TSharedPtr<IPropertyUtilities> Utils = PropertyUtilities.Pin())
	{
		Utils->RequestForceRefresh();
	}
}

ALevelProcessController* FPushBoxFlowNodeCustomization::ResolveProcessController() const
{
	if (!ProcessControllerHandle.IsValid() || !ProcessControllerHandle->IsValidHandle())
	{
		return nullptr;
	}

	TArray<void*> RawData;
	ProcessControllerHandle->AccessRawData(RawData);
	for (void* RawPtr : RawData)
	{
		if (!RawPtr)
		{
			continue;
		}

		const TSoftObjectPtr<ALevelProcessController>* SoftControllerPtr = static_cast<TSoftObjectPtr<ALevelProcessController>*>(RawPtr);
		if (!SoftControllerPtr)
		{
			continue;
		}

		if (ALevelProcessController* Controller = SoftControllerPtr->Get())
		{
			return Controller;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
