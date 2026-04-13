// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoxCell.h"
#include "BoxActor.h"
#include "Components/StaticMeshComponent.h"

ABoxCell::ABoxCell()
{
	bBlocksMovement = false;
	BoxActorClass = ABoxActor::StaticClass();

	BoxPreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxPreviewMesh"));
	BoxPreviewMeshComponent->SetupAttachment(SceneRoot);
	BoxPreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxPreviewMeshComponent->SetVisibility(true);
	BoxPreviewMeshComponent->SetHiddenInGame(true);
	BoxPreviewMeshComponent->SetReceivesDecals(false);

	if (CellMeshComponent)
	{
		CellMeshComponent->SetReceivesDecals(false);
	}
}

UStaticMesh* ABoxCell::GetBoxPreviewMesh() const
{
	return BoxPreviewMeshComponent ? BoxPreviewMeshComponent->GetStaticMesh() : nullptr;
}

FTransform ABoxCell::GetBoxPreviewMeshRelativeTransform() const
{
	return BoxPreviewMeshComponent ? BoxPreviewMeshComponent->GetRelativeTransform() : FTransform::Identity;
}
