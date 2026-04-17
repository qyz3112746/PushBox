// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxFlowDirector.h"

#include "LevelProcessController.h"
#include "PushBox.h"
#include "PushBoxFlowDataAsset.h"

APushBoxFlowDirector::APushBoxFlowDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	FlowDataAsset = nullptr;
	bAutoStartOnBeginPlay = true;
	StartMode = EPushBoxFlowStartMode::UseFlowDefault;
	DebugNodeIndex = 0;
	DebugLevelIndexInNode = 0;
	CurrentNodeIndex = INDEX_NONE;
	CurrentLevelIndexInNode = INDEX_NONE;
	ActiveProcessController = nullptr;
}

void APushBoxFlowDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartOnBeginPlay)
	{
		StartFlow();
	}
}

bool APushBoxFlowDirector::StartFlow()
{
	if (!FlowDataAsset)
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::StartFlow failed: FlowDataAsset is null."));
		return false;
	}

	if (FlowDataAsset->Nodes.Num() == 0)
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::StartFlow failed: FlowDataAsset has no nodes."));
		return false;
	}

	const int32 StartNodeIndex = StartMode == EPushBoxFlowStartMode::DebugNodeLevel ? DebugNodeIndex : 0;
	const int32 StartLevelIndex = StartMode == EPushBoxFlowStartMode::DebugNodeLevel ? DebugLevelIndexInNode : 0;
	return ActivateNodeLevel(StartNodeIndex, StartLevelIndex);
}

bool APushBoxFlowDirector::ConfirmAdvance()
{
	if (!ActiveProcessController)
	{
		return false;
	}

	if (!ActiveProcessController->IsPendingTransition())
	{
		return false;
	}

	if (!FlowDataAsset || !FlowDataAsset->Nodes.IsValidIndex(CurrentNodeIndex))
	{
		return false;
	}

	const int32 NextLevelIndex = CurrentLevelIndexInNode + 1;
	if (ActiveProcessController->LevelSequence.IsValidIndex(NextLevelIndex))
	{
		const bool bLoaded = ActiveProcessController->LoadLevelByIndex(NextLevelIndex);
		if (bLoaded)
		{
			CurrentLevelIndexInNode = NextLevelIndex;
		}
		return bLoaded;
	}

	ActiveProcessController->CancelPendingTransition();

	const int32 NextNodeIndex = CurrentNodeIndex + 1;
	if (FlowDataAsset->Nodes.IsValidIndex(NextNodeIndex))
	{
		return ActivateNodeLevel(NextNodeIndex, 0);
	}

	OnFlowCompleted.Broadcast();
	return true;
}

bool APushBoxFlowDirector::ConfirmReplayCurrent()
{
	if (!ActiveProcessController)
	{
		return false;
	}

	if (!ActiveProcessController->IsPendingTransition())
	{
		return false;
	}

	return ActiveProcessController->ConfirmRestartCurrentLevel();
}

bool APushBoxFlowDirector::ActivateNodeLevel(int32 NodeIndex, int32 LevelIndexInNode)
{
	if (!FlowDataAsset || !FlowDataAsset->Nodes.IsValidIndex(NodeIndex))
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::ActivateNodeLevel failed: invalid node index %d."), NodeIndex);
		return false;
	}

	const FPushBoxFlowNode& Node = FlowDataAsset->Nodes[NodeIndex];
	ALevelProcessController* Controller = Node.ProcessController.Get();
	if (!Controller)
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::ActivateNodeLevel failed: node %d has invalid ProcessController reference."), NodeIndex);
		return false;
	}
	Controller->bAutoStartOnBeginPlay = false;

	if (!Controller->LevelSequence.IsValidIndex(LevelIndexInNode))
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::ActivateNodeLevel failed: controller '%s' invalid level index %d (LevelSequence Num=%d)."),
			*GetNameSafe(Controller),
			LevelIndexInNode,
			Controller->LevelSequence.Num());
		return false;
	}

	BindActiveController(Controller);

	const bool bLoaded = Controller->LoadLevelByIndex(LevelIndexInNode);
	if (!bLoaded)
	{
		return false;
	}

	CurrentNodeIndex = NodeIndex;
	CurrentLevelIndexInNode = LevelIndexInNode;
	return true;
}

void APushBoxFlowDirector::BindActiveController(ALevelProcessController* Controller)
{
	if (ActiveProcessController)
	{
		ActiveProcessController->OnFlowCompleted.RemoveDynamic(this, &APushBoxFlowDirector::HandleActiveControllerFlowCompleted);
	}

	ActiveProcessController = Controller;
	if (ActiveProcessController)
	{
		ActiveProcessController->OnFlowCompleted.RemoveDynamic(this, &APushBoxFlowDirector::HandleActiveControllerFlowCompleted);
		ActiveProcessController->OnFlowCompleted.AddDynamic(this, &APushBoxFlowDirector::HandleActiveControllerFlowCompleted);
	}
}

void APushBoxFlowDirector::HandleActiveControllerFlowCompleted()
{
	// Keep the director as the single completion source.
}
