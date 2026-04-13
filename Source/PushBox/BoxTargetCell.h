// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridCellBase.h"
#include "BoxTargetCell.generated.h"

class ABoxActor;
class UNiagaraComponent;

UCLASS(Blueprintable)
class ABoxTargetCell : public AGridCellBase
{
	GENERATED_BODY()

public:
	ABoxTargetCell();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Target")
	void SetMatchedState(bool bIsMatched);

	UFUNCTION(BlueprintPure, Category = "PushBox|Target")
	bool IsBoxAccepted(const ABoxActor* BoxActor) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PushBox|Target")
	TArray<TSubclassOf<ABoxActor>> RequiredBoxActorTypes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell")
	UNiagaraComponent* MatchedNiagaraComponent;

private:
	bool bMatched;
};
