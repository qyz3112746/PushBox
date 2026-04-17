// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxFlowDirector.h"

#include "LevelProcessController.h"
#include "PushBox.h"
#include "PushBoxFlowDataAsset.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
	ALevelProcessController* Controller = ResolveNodeController(Node);
	if (!Controller)
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::ActivateNodeLevel failed: node %d could not resolve ProcessController (NodeId=%s, Ref=%s)."),
			NodeIndex,
			*Node.NodeId.ToString(),
			*Node.ProcessController.ToSoftObjectPath().ToString());
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

	const bool bLoaded = Controller->LoadLevelByIndex(LevelIndexInNode);
	if (!bLoaded)
	{
		return false;
	}

	BindActiveController(Controller);
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

ALevelProcessController* APushBoxFlowDirector::ResolveNodeController(const FPushBoxFlowNode& Node) const
{
	// 1) Direct instance reference (best case).
	if (ALevelProcessController* DirectController = Node.ProcessController.Get())
	{
		if (!GetWorld() || DirectController->GetWorld() == GetWorld())
		{
			return DirectController;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 2) NodeId tag fallback.
	if (!Node.NodeId.IsNone())
	{
		for (TActorIterator<ALevelProcessController> It(World); It; ++It)
		{
			ALevelProcessController* Candidate = *It;
			if (!Candidate)
			{
				continue;
			}

			if (Candidate->ActorHasTag(Node.NodeId) || Candidate->GetFName() == Node.NodeId)
			{
				return Candidate;
			}
		}
	}

	// 3) Soft reference might be a Blueprint class asset; resolve by class in current world.
	UClass* DesiredClass = nullptr;
	const FSoftObjectPath RefPath = Node.ProcessController.ToSoftObjectPath();
	if (RefPath.IsValid())
	{
		if (UObject* RefObject = RefPath.TryLoad())
		{
			if (UClass* RefClass = Cast<UClass>(RefObject))
			{
				if (RefClass->IsChildOf(ALevelProcessController::StaticClass()))
				{
					DesiredClass = RefClass;
				}
			}
			else if (const UBlueprint* RefBlueprint = Cast<UBlueprint>(RefObject))
			{
				if (RefBlueprint->GeneratedClass && RefBlueprint->GeneratedClass->IsChildOf(ALevelProcessController::StaticClass()))
				{
					DesiredClass = RefBlueprint->GeneratedClass;
				}
			}
		}
	}

	if (DesiredClass)
	{
		TArray<ALevelProcessController*> Matches;
		for (TActorIterator<ALevelProcessController> It(World); It; ++It)
		{
			ALevelProcessController* Candidate = *It;
			if (!Candidate || !Candidate->IsA(DesiredClass))
			{
				continue;
			}

			if (!Node.NodeId.IsNone() && Candidate->ActorHasTag(Node.NodeId))
			{
				return Candidate;
			}

			Matches.Add(Candidate);
		}

		if (Matches.Num() > 0)
		{
			// If there are multiple same-class controllers and we already have an active one,
			// prefer switching to a different instance to enable node-to-node transitions.
			if (Matches.Num() > 1 && ActiveProcessController)
			{
				for (ALevelProcessController* Candidate : Matches)
				{
					if (Candidate && Candidate != ActiveProcessController)
					{
						return Candidate;
					}
				}
			}

			return Matches[0];
		}
	}

	return nullptr;
}
