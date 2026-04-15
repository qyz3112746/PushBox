// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxEditorBridgeSubsystem.h"

void UPushBoxEditorBridgeSubsystem::BroadcastApplyCellDisplayRequested(const FCellDisplay& Payload, const FString& PropertyPath)
{
	OnApplyCellDisplayRequested.Broadcast(Payload, PropertyPath);
}
