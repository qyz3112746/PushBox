// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IDetailChildrenBuilder;
class IPropertyHandle;

class FCellDisplayCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	FReply HandleApplyClicked() const;

	TSharedPtr<IPropertyHandle> RootHandle;
	TSharedPtr<IPropertyHandle> CellTypeHandle;
	TSharedPtr<IPropertyHandle> BGColorHandle;
	TSharedPtr<IPropertyHandle> IconHandle;
};
