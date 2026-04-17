// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxEditorModule.h"

#include "CellDisplayCustomization.h"
#include "PushBoxFlowDataAsset.h"
#include "PushBoxFlowNodeCustomization.h"
#include "PushBoxLevelSequenceEntryCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "LevelProcessController.h"

void FPushBoxEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(
		"CellDisplay",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCellDisplayCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(
		FPushBoxFlowNode::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPushBoxFlowNodeCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(
		FPushBoxLevelSequenceEntry::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPushBoxLevelSequenceEntryCustomization::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FPushBoxEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout("CellDisplay");
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout(FPushBoxFlowNode::StaticStruct()->GetFName());
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout(FPushBoxLevelSequenceEntry::StaticStruct()->GetFName());
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

IMPLEMENT_MODULE(FPushBoxEditorModule, PushBoxEditor)
