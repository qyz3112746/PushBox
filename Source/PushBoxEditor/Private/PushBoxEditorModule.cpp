// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxEditorModule.h"

#include "CellDisplayCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

void FPushBoxEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(
		"CellDisplay",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCellDisplayCustomization::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FPushBoxEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout("CellDisplay");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

IMPLEMENT_MODULE(FPushBoxEditorModule, PushBoxEditor)
