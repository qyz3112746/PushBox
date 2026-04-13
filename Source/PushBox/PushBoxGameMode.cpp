// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxGameMode.h"
#include "PushBoxPlayerController.h"
#include "UObject/ConstructorHelpers.h"

APushBoxGameMode::APushBoxGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = APushBoxPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
