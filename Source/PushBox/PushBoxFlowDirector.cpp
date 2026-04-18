// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxFlowDirector.h"

#include "Algo/Sort.h"
#include "LevelProcessController.h"
#include "PushBox.h"
#include "PushBoxFlowDataAsset.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/SoftObjectPath.h"

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

bool APushBoxFlowDirector::StartFlowAtNodeLevel(int32 NodeIndex, int32 LevelIndexInNode)
{
	return ActivateNodeLevel(NodeIndex, LevelIndexInNode);
}

bool APushBoxFlowDirector::ActivateNodeLevel(int32 NodeIndex, int32 LevelIndexInNode)
{
	if (!FlowDataAsset || !FlowDataAsset->Nodes.IsValidIndex(NodeIndex))
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::ActivateNodeLevel failed: invalid node index %d."), NodeIndex);
		return false;
	}

	const FPushBoxFlowNode& Node = FlowDataAsset->Nodes[NodeIndex];
	ALevelProcessController* Controller = ResolveNodeController(Node, NodeIndex);
	if (!Controller)
	{
		UE_LOG(LogPushBox, Error, TEXT("FlowDirector::ActivateNodeLevel failed: node %d could not resolve ProcessController (Ref=%s)."),
			NodeIndex,
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
		ActiveProcessController->OnPendingTransitionStarted.RemoveDynamic(this, &APushBoxFlowDirector::HandleActiveControllerPendingTransitionStarted);
		ActiveProcessController->OnFlowCompleted.RemoveDynamic(this, &APushBoxFlowDirector::HandleActiveControllerFlowCompleted);
	}

	ActiveProcessController = Controller;
	if (ActiveProcessController)
	{
		ActiveProcessController->OnPendingTransitionStarted.RemoveDynamic(this, &APushBoxFlowDirector::HandleActiveControllerPendingTransitionStarted);
		ActiveProcessController->OnPendingTransitionStarted.AddDynamic(this, &APushBoxFlowDirector::HandleActiveControllerPendingTransitionStarted);
		ActiveProcessController->OnFlowCompleted.RemoveDynamic(this, &APushBoxFlowDirector::HandleActiveControllerFlowCompleted);
		ActiveProcessController->OnFlowCompleted.AddDynamic(this, &APushBoxFlowDirector::HandleActiveControllerFlowCompleted);
	}
}

void APushBoxFlowDirector::HandleActiveControllerPendingTransitionStarted(bool bLevelPassed, int32 CurrentIndex, FName CurrentLevelId)
{
	OnPendingTransitionStarted.Broadcast(bLevelPassed, CurrentIndex, CurrentLevelId);
}

void APushBoxFlowDirector::HandleActiveControllerFlowCompleted()
{
	// Keep the director as the single completion source.
}

ALevelProcessController* APushBoxFlowDirector::ResolveNodeController(const FPushBoxFlowNode& Node, int32 NodeIndex) const
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

	const FSoftObjectPath RefPath = Node.ProcessController.ToSoftObjectPath();

	// 2) Soft actor reference by object-name fallback (handles PIE world duplication).
	if (RefPath.IsValid())
	{
		FString TargetActorName;
		const FString SubPath = RefPath.GetSubPathString(); // e.g. PersistentLevel.BP_LevelProcessController_C_0
		if (!SubPath.IsEmpty())
		{
			int32 LastDot = INDEX_NONE;
			if (SubPath.FindLastChar(TEXT('.'), LastDot) && LastDot + 1 < SubPath.Len())
			{
				TargetActorName = SubPath.Mid(LastDot + 1);
			}
		}

		if (TargetActorName.IsEmpty())
		{
			FString RefString = RefPath.ToString();
			int32 LastDot = INDEX_NONE;
			if (RefString.FindLastChar(TEXT('.'), LastDot) && LastDot + 1 < RefString.Len())
			{
				TargetActorName = RefString.Mid(LastDot + 1);
			}
		}

		if (!TargetActorName.IsEmpty())
		{
			for (TActorIterator<ALevelProcessController> It(World); It; ++It)
			{
				ALevelProcessController* Candidate = *It;
				if (!Candidate)
				{
					continue;
				}

				if (Candidate->GetName() == TargetActorName || Candidate->GetPathName().EndsWith(TargetActorName))
				{
					return Candidate;
				}
			}
		}
	}

	// 3) Soft reference might be a Blueprint class asset; resolve by class in current world.
	UClass* DesiredClass = nullptr;
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

				Matches.Add(Candidate);
			}

		if (Matches.Num() > 0)
		{
			Algo::Sort(Matches, [](const ALevelProcessController* A, const ALevelProcessController* B)
			{
				return GetNameSafe(A) < GetNameSafe(B);
			});

			// For duplicate node definitions that point to the same controller reference/class,
			// map by ordinal occurrence in Nodes[] so each node can bind to a different placed controller.
			int32 DuplicateOrdinal = 0;
			if (FlowDataAsset && FlowDataAsset->Nodes.IsValidIndex(NodeIndex))
			{
				const FSoftObjectPath CurrentRefPath = Node.ProcessController.ToSoftObjectPath();
				const UClass* CurrentDesiredClass = DesiredClass;
				for (int32 ScanIndex = 0; ScanIndex < NodeIndex; ++ScanIndex)
				{
					const FPushBoxFlowNode& ScanNode = FlowDataAsset->Nodes[ScanIndex];
					const FSoftObjectPath ScanRefPath = ScanNode.ProcessController.ToSoftObjectPath();
					if (CurrentRefPath.IsValid() && ScanRefPath.IsValid() && ScanRefPath == CurrentRefPath)
					{
						++DuplicateOrdinal;
						continue;
					}

					UObject* ScanObject = ScanRefPath.IsValid() ? ScanRefPath.TryLoad() : nullptr;
					UClass* ScanClass = nullptr;
					if (UClass* RefClass = Cast<UClass>(ScanObject))
					{
						ScanClass = RefClass;
					}
					else if (const UBlueprint* RefBlueprint = Cast<UBlueprint>(ScanObject))
					{
						ScanClass = RefBlueprint->GeneratedClass;
					}

					if (ScanClass && CurrentDesiredClass && ScanClass == CurrentDesiredClass)
					{
						++DuplicateOrdinal;
					}
				}
			}

			if (DuplicateOrdinal >= 0 && DuplicateOrdinal < Matches.Num())
			{
				return Matches[DuplicateOrdinal];
			}

			// Backward-compatible fallback.
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
