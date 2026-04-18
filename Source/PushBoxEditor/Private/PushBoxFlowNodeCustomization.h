// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class ALevelProcessController;
class IPropertyHandle;
class IPropertyUtilities;

class FPushBoxFlowNodeCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	void HandleProcessControllerChanged();
	ALevelProcessController* ResolveProcessController() const;

	TSharedPtr<IPropertyHandle> RootHandle;
	TSharedPtr<IPropertyHandle> ProcessControllerHandle;
	TWeakPtr<IPropertyUtilities> PropertyUtilities;
};
