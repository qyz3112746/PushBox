// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxEditorBridgeSubsystem.h"
#include "LevelDataGridEditorWidget.h"

void UPushBoxEditorBridgeSubsystem::BroadcastApplyCellDisplayRequested(const FCellDisplay& Payload, const FString& PropertyPath)
{
	OnApplyCellDisplayRequested.Broadcast(Payload, PropertyPath);
	ApplyToActiveMapEditor(Payload);
}

void UPushBoxEditorBridgeSubsystem::RegisterActiveMapEditor(ULevelDataGridEditorWidget* InMapEditor)
{
	ActiveMapEditor = InMapEditor;
}

void UPushBoxEditorBridgeSubsystem::UnregisterActiveMapEditor(ULevelDataGridEditorWidget* InMapEditor)
{
	if (ActiveMapEditor == InMapEditor)
	{
		ActiveMapEditor = nullptr;
	}
}

bool UPushBoxEditorBridgeSubsystem::ApplyToActiveMapEditor(const FCellDisplay& Payload)
{
	if (!ActiveMapEditor)
	{
		return false;
	}

	return ActiveMapEditor->ApplyCellDisplayToSelection(Payload);
}
