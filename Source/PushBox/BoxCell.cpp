// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoxCell.h"
#include "BoxActor.h"

ABoxCell::ABoxCell()
{
	bBlocksMovement = false;
	BoxActorClass = ABoxActor::StaticClass();
	BoxActorMesh = nullptr;
}
