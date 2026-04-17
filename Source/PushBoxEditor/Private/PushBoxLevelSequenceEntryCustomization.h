// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"

class ALevelProcessController;
class IPropertyHandle;

class FPushBoxLevelSequenceEntryCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	FReply HandlePreviewClicked() const;
	ALevelProcessController* ResolveOwnerProcessController() const;
	int32 ResolveSequenceIndex() const;

private:
	TSharedPtr<IPropertyHandle> RootHandle;
	TSharedPtr<IPropertyHandle> LevelDataHandle;
};

