// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GridCellBase.h"
#include "BoxTargetCell.generated.h"

class UNiagaraComponent;

UCLASS(Blueprintable)
class ABoxTargetCell : public AGridCellBase
{
	GENERATED_BODY()

public:
	ABoxTargetCell();

	UFUNCTION(BlueprintCallable, Category = "PushBox|Target")
	void SetMatchedState(bool bIsMatched);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell")
	UNiagaraComponent* MatchedNiagaraComponent;

private:
	bool bMatched;
};
