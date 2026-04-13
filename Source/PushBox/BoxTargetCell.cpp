// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoxTargetCell.h"
#include "BoxActor.h"
#include "NiagaraComponent.h"

ABoxTargetCell::ABoxTargetCell()
{
	bMatched = false;

	MatchedNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MatchedNiagara"));
	MatchedNiagaraComponent->SetupAttachment(SceneRoot);
	MatchedNiagaraComponent->bAutoActivate = false;
	MatchedNiagaraComponent->SetVisibility(false);
}

void ABoxTargetCell::SetMatchedState(bool bIsMatched)
{
	bMatched = bIsMatched;

	if (CellNiagaraComponent)
	{
		CellNiagaraComponent->SetVisibility(!bMatched);
		CellNiagaraComponent->SetComponentTickEnabled(!bMatched);
		if (bMatched)
		{
			CellNiagaraComponent->Deactivate();
		}
		else
		{
			CellNiagaraComponent->Activate(true);
		}
	}

	if (MatchedNiagaraComponent)
	{
		MatchedNiagaraComponent->SetVisibility(bMatched);
		MatchedNiagaraComponent->SetComponentTickEnabled(bMatched);
		if (bMatched)
		{
			MatchedNiagaraComponent->Activate(true);
		}
		else
		{
			MatchedNiagaraComponent->Deactivate();
		}
	}
}

bool ABoxTargetCell::IsBoxAccepted(const ABoxActor* BoxActor) const
{
	if (!BoxActor || RequiredBoxActorTypes.Num() == 0)
	{
		return false;
	}

	UClass* BoxClass = BoxActor->GetClass();
	for (const TSubclassOf<ABoxActor>& RequiredType : RequiredBoxActorTypes)
	{
		if (RequiredType.Get() == BoxClass)
		{
			return true;
		}
	}

	return false;
}
