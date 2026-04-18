// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelSequenceEntryCustomization.h"

#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Blueprint.h"
#include "EngineUtils.h"
#include "IDetailChildrenBuilder.h"
#include "LevelProcessController.h"
#include "PlayInEditorDataTypes.h"
#include "PropertyHandle.h"
#include "PushBoxFlowDirector.h"
#include "PushBoxFlowDataAsset.h"
#include "PushBoxLevelData.h"
#include "PushBoxLevelPreviewSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

namespace
{
struct FPendingFlowTestRequest
{
	FSoftObjectPath DirectorPath;
	TSoftObjectPtr<UPushBoxFlowDataAsset> FlowDataAsset;
	int32 NodeIndex = INDEX_NONE;
	int32 SequenceIndex = INDEX_NONE;
};

TOptional<FPendingFlowTestRequest> GPendingFlowTestRequest;
FDelegateHandle GPostPIEStartedHandle;

void ConsumePendingFlowTestRequest(bool bIsSimulating)
{
	if (bIsSimulating || !GPendingFlowTestRequest.IsSet() || !GEditor || !GEditor->PlayWorld)
	{
		return;
	}

	const FPendingFlowTestRequest Request = GPendingFlowTestRequest.GetValue();
	GPendingFlowTestRequest.Reset();

	APushBoxFlowDirector* MatchedDirector = nullptr;
	for (TActorIterator<APushBoxFlowDirector> It(GEditor->PlayWorld); It; ++It)
	{
		APushBoxFlowDirector* Candidate = *It;
		if (!Candidate)
		{
			continue;
		}

		const bool bMatchByPath =
			Request.DirectorPath.IsValid() &&
			(Candidate->GetPathName().EndsWith(Request.DirectorPath.GetSubPathString()) ||
			 Candidate->GetPathName() == Request.DirectorPath.ToString());
		const bool bMatchByAsset =
			Request.FlowDataAsset.IsValid() &&
			Candidate->GetFlowDataAsset() == Request.FlowDataAsset.Get();
		if (bMatchByPath || bMatchByAsset)
		{
			MatchedDirector = Candidate;
			break;
		}

		if (!MatchedDirector)
		{
			MatchedDirector = Candidate;
		}
	}

	if (MatchedDirector)
	{
		MatchedDirector->StartFlowAtNodeLevel(Request.NodeIndex, Request.SequenceIndex);
	}
}

void EnsurePostPIEStartedHandler()
{
	if (GPostPIEStartedHandle.IsValid())
	{
		return;
	}
	GPostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddStatic(&ConsumePendingFlowTestRequest);
}
} // namespace

TSharedRef<IPropertyTypeCustomization> FPushBoxLevelSequenceEntryCustomization::MakeInstance()
{
	return MakeShared<FPushBoxLevelSequenceEntryCustomization>();
}

void FPushBoxLevelSequenceEntryCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	RootHandle = StructPropertyHandle;
	LevelDataHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPushBoxLevelSequenceEntry, LevelData));

	HeaderRow
	.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()]
	.ValueContent()
	.MinDesiredWidth(320.0f)
	.MaxDesiredWidth(1400.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			LevelDataHandle.IsValid()
				? LevelDataHandle->CreatePropertyValueWidget()
				: StructPropertyHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 0.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("PushBoxLevelSequenceEntryCustomization", "Preview", "Preview"))
			.OnClicked(FOnClicked::CreateSP(this, &FPushBoxLevelSequenceEntryCustomization::HandlePreviewClicked))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(6.0f, 0.0f, 0.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("PushBoxLevelSequenceEntryCustomization", "Test", "Test"))
			.OnClicked(FOnClicked::CreateSP(this, &FPushBoxLevelSequenceEntryCustomization::HandleTestClicked))
		]
	];
}

void FPushBoxLevelSequenceEntryCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

FReply FPushBoxLevelSequenceEntryCustomization::HandlePreviewClicked() const
{
	if (!GEditor || !LevelDataHandle.IsValid())
	{
		return FReply::Handled();
	}

	UObject* LevelDataObject = nullptr;
	if (LevelDataHandle->GetValue(LevelDataObject) != FPropertyAccess::Success)
	{
		return FReply::Handled();
	}

	UPushBoxLevelData* LevelData = Cast<UPushBoxLevelData>(LevelDataObject);
	ALevelProcessController* ProcessController = ResolveOwnerProcessController();
	const int32 SequenceIndex = ResolveSequenceIndex();
	if (!ProcessController || !LevelData || SequenceIndex == INDEX_NONE)
	{
		return FReply::Handled();
	}

	if (UPushBoxLevelPreviewSubsystem* PreviewSubsystem = GEditor->GetEditorSubsystem<UPushBoxLevelPreviewSubsystem>())
	{
		PreviewSubsystem->StartPreview(ProcessController, LevelData, SequenceIndex);
	}

	return FReply::Handled();
}

FReply FPushBoxLevelSequenceEntryCustomization::HandleTestClicked() const
{
	if (!GEditor || !LevelDataHandle.IsValid())
	{
		return FReply::Handled();
	}

	UObject* LevelDataObject = nullptr;
	LevelDataHandle->GetValue(LevelDataObject);
	UPushBoxLevelData* LevelData = Cast<UPushBoxLevelData>(LevelDataObject);

	int32 SequenceIndex = ResolveSequenceIndex();
	ALevelProcessController* ProcessController = ResolveOwnerProcessController();
	APushBoxFlowDirector* EditorDirector = ResolveEditorFlowDirector(ProcessController, INDEX_NONE);
	if (!EditorDirector)
	{
		UE_LOG(LogTemp, Warning, TEXT("Test clicked ignored: could not resolve editor FlowDirector."));
		return FReply::Handled();
	}

	if (SequenceIndex == INDEX_NONE)
	{
		SequenceIndex = ResolveSequenceIndexFallback(ProcessController, LevelData);
	}

	const int32 NodeIndex = ResolveNodeIndexInDirector(EditorDirector, ProcessController);

	if (NodeIndex == INDEX_NONE || SequenceIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Test clicked ignored: failed to resolve controller/node/sequence. Controller=%s Node=%d Seq=%d Level=%s"),
			*GetNameSafe(ProcessController), NodeIndex, SequenceIndex, *GetNameSafe(LevelData));
		return FReply::Handled();
	}

	StartOrSwitchToTestPlay(EditorDirector, NodeIndex, SequenceIndex);

	return FReply::Handled();
}

ALevelProcessController* FPushBoxLevelSequenceEntryCustomization::ResolveOwnerProcessController() const
{
	if (!RootHandle.IsValid())
	{
		return nullptr;
	}

	TArray<UObject*> OuterObjects;
	RootHandle->GetOuterObjects(OuterObjects);
	for (UObject* Outer : OuterObjects)
	{
		if (ALevelProcessController* Controller = Cast<ALevelProcessController>(Outer))
		{
			return Controller;
		}
	}

	// Fallback for FlowAsset details: resolve parent Node.ProcessController field.
	TSharedPtr<IPropertyHandle> NodeHandle = RootHandle->GetParentHandle();
	if (NodeHandle.IsValid())
	{
		NodeHandle = NodeHandle->GetParentHandle();
	}
	if (NodeHandle.IsValid())
	{
		const TSharedPtr<IPropertyHandle> ControllerHandle = NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPushBoxFlowNode, ProcessController));
		if (ControllerHandle.IsValid())
		{
			UObject* ControllerObject = nullptr;
			if (ControllerHandle->GetValue(ControllerObject) == FPropertyAccess::Success)
			{
				if (ALevelProcessController* Controller = Cast<ALevelProcessController>(ControllerObject))
				{
					return Controller;
				}
			}
		}
	}

	return nullptr;
}

int32 FPushBoxLevelSequenceEntryCustomization::ResolveSequenceIndex() const
{
	if (!RootHandle.IsValid())
	{
		return INDEX_NONE;
	}

	// Preferred: array element index from property-handle hierarchy.
	const int32 RootArrayIndex = RootHandle->GetIndexInArray();
	if (RootArrayIndex != INDEX_NONE)
	{
		return RootArrayIndex;
	}

	auto ExtractIndexForToken = [](const FString& InPath, const TCHAR* InToken) -> int32
	{
		const FString TokenString = InToken;
		int32 TokenIndex = InPath.Find(TokenString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (TokenIndex == INDEX_NONE)
		{
			return INDEX_NONE;
		}
		TokenIndex += TokenString.Len();

		int32 CloseBracketIndex = INDEX_NONE;
		for (int32 I = TokenIndex; I < InPath.Len(); ++I)
		{
			if (InPath[I] == TEXT(']'))
			{
				CloseBracketIndex = I;
				break;
			}
		}
		if (CloseBracketIndex == INDEX_NONE || CloseBracketIndex <= TokenIndex)
		{
			return INDEX_NONE;
		}

		const FString IndexString = InPath.Mid(TokenIndex, CloseBracketIndex - TokenIndex);
		return FCString::Atoi(*IndexString);
	};

	// 1) Try current handle path.
	int32 Index = ExtractIndexForToken(RootHandle->GeneratePathToProperty(), TEXT("LevelSequence["));
	if (Index != INDEX_NONE)
	{
		return Index;
	}

	// 2) Walk parent chain (external object details can move the useful token to parents).
	TSharedPtr<IPropertyHandle> Handle = RootHandle;
	while (Handle.IsValid())
	{
		Index = ExtractIndexForToken(Handle->GeneratePathToProperty(), TEXT("LevelSequence["));
		if (Index != INDEX_NONE)
		{
			return Index;
		}
		Handle = Handle->GetParentHandle();
	}

	return INDEX_NONE;
}


int32 FPushBoxLevelSequenceEntryCustomization::ResolveNodeIndexInDirector(APushBoxFlowDirector* Director, ALevelProcessController* ProcessController) const
{
	if (!Director || !Director->GetFlowDataAsset() || !ProcessController)
	{
		return INDEX_NONE;
	}

	const TArray<FPushBoxFlowNode>& Nodes = Director->GetFlowDataAsset()->Nodes;
	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex)
	{
		const FPushBoxFlowNode& Node = Nodes[NodeIndex];
		if (Node.ProcessController.Get() == ProcessController)
		{
			return NodeIndex;
		}

		// Class asset match fallback (node may store BP class soft-ref, not placed instance).
		const FSoftObjectPath RefPath = Node.ProcessController.ToSoftObjectPath();
		if (RefPath.IsValid())
		{
			if (UObject* RefObject = RefPath.TryLoad())
			{
				UClass* DesiredClass = nullptr;
				if (UClass* RefClass = Cast<UClass>(RefObject))
				{
					DesiredClass = RefClass;
				}
				else if (const UBlueprint* RefBlueprint = Cast<UBlueprint>(RefObject))
				{
					DesiredClass = RefBlueprint->GeneratedClass;
				}

				if (DesiredClass && ProcessController->IsA(DesiredClass))
				{
					return NodeIndex;
				}
			}
		}
	}

	return INDEX_NONE;
}

int32 FPushBoxLevelSequenceEntryCustomization::ResolveSequenceIndexFallback(ALevelProcessController* ProcessController, UPushBoxLevelData* LevelData) const
{
	if (!ProcessController || !LevelData)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < ProcessController->LevelSequence.Num(); ++Index)
	{
		if (ProcessController->LevelSequence[Index].LevelData == LevelData)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

APushBoxFlowDirector* FPushBoxLevelSequenceEntryCustomization::ResolveEditorFlowDirector(ALevelProcessController* ProcessController, int32 NodeIndex) const
{
	if (!GEditor)
	{
		return nullptr;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return nullptr;
	}

	APushBoxFlowDirector* Fallback = nullptr;
	for (TActorIterator<APushBoxFlowDirector> It(EditorWorld); It; ++It)
	{
		APushBoxFlowDirector* Candidate = *It;
		if (!Candidate)
		{
			continue;
		}

		Fallback = Candidate;
		if (!Candidate->GetFlowDataAsset() || !Candidate->GetFlowDataAsset()->Nodes.IsValidIndex(NodeIndex))
		{
			continue;
		}

		const FPushBoxFlowNode& Node = Candidate->GetFlowDataAsset()->Nodes[NodeIndex];
		ALevelProcessController* NodeController = Node.ProcessController.Get();
		if (NodeController == ProcessController)
		{
			return Candidate;
		}
	}

	return Fallback;
}

void FPushBoxLevelSequenceEntryCustomization::StartOrSwitchToTestPlay(APushBoxFlowDirector* EditorDirector, int32 NodeIndex, int32 SequenceIndex) const
{
	if (!GEditor)
	{
		return;
	}

	if (GEditor->IsPlayingSessionInEditor() && GEditor->PlayWorld)
	{
		for (TActorIterator<APushBoxFlowDirector> It(GEditor->PlayWorld); It; ++It)
		{
			APushBoxFlowDirector* PieDirector = *It;
			if (!PieDirector)
			{
				continue;
			}

			const bool bMatchAsset =
				EditorDirector &&
				EditorDirector->GetFlowDataAsset() &&
				PieDirector->GetFlowDataAsset() == EditorDirector->GetFlowDataAsset();
			if (bMatchAsset || !EditorDirector)
			{
				PieDirector->StartFlowAtNodeLevel(NodeIndex, SequenceIndex);
				return;
			}
		}
		return;
	}

	if (!GEditor->IsPlaySessionInProgress())
	{
		EnsurePostPIEStartedHandler();
		FPendingFlowTestRequest& PendingRequest = GPendingFlowTestRequest.Emplace();
		PendingRequest.DirectorPath = EditorDirector ? FSoftObjectPath(EditorDirector) : FSoftObjectPath();
		PendingRequest.FlowDataAsset = EditorDirector ? EditorDirector->GetFlowDataAsset() : nullptr;
		PendingRequest.NodeIndex = NodeIndex;
		PendingRequest.SequenceIndex = SequenceIndex;

		FRequestPlaySessionParams PlayParams;
		PlayParams.WorldType = EPlaySessionWorldType::PlayInEditor;
		GEditor->RequestPlaySession(PlayParams);
	}
}

