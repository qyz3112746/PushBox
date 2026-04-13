// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoxCell.h"
#include "BoxActor.h"

ABoxCell::ABoxCell()
{
	bBlocksMovement = false;
	BoxSpawnClass = ABoxActor::StaticClass();
	SpawnedBox = nullptr;

	if (CellMeshComponent)
	{
		CellMeshComponent->SetReceivesDecals(false);
	}
}

void ABoxCell::BeginPlay()
{
	Super::BeginPlay();
	SpawnedBox = SpawnBoxAt(BoxSpawnClass, GetGridCoord());
}
