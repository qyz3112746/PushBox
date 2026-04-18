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
	FReply HandleTestClicked() const;
	ALevelProcessController* ResolveOwnerProcessController() const;
	int32 ResolveSequenceIndex() const;
	class APushBoxFlowDirector* ResolveEditorFlowDirector(ALevelProcessController* ProcessController, int32 NodeIndex) const;
	int32 ResolveNodeIndexInDirector(class APushBoxFlowDirector* Director, ALevelProcessController* ProcessController) const;
	int32 ResolveSequenceIndexFallback(ALevelProcessController* ProcessController, class UPushBoxLevelData* LevelData) const;
	void ConfigureEditorDirectorForTest(class APushBoxFlowDirector* Director, int32 NodeIndex, int32 SequenceIndex) const;
	void StartOrSwitchToTestPlay(class APushBoxFlowDirector* EditorDirector, int32 NodeIndex, int32 SequenceIndex) const;

private:
	TSharedPtr<IPropertyHandle> RootHandle;
	TSharedPtr<IPropertyHandle> LevelDataHandle;
};

