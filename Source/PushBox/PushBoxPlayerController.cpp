// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxPlayerController.h"
#include "LevelProcessController.h"
#include "PushBoxLevelRuntime.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"

DEFINE_LOG_CATEGORY(LogPushBoxController);

APushBoxPlayerController::APushBoxPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedProcessController = nullptr;
	CachedLevelRuntime = nullptr;
}

void APushBoxPlayerController::BeginPlay()
{
	Super::BeginPlay();
	CachedLevelRuntime = ResolveLevelRuntime();
}

void APushBoxPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::W, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveUp);
	InputComponent->BindKey(EKeys::Up, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveUp);

	InputComponent->BindKey(EKeys::S, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveDown);
	InputComponent->BindKey(EKeys::Down, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveDown);

	InputComponent->BindKey(EKeys::A, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveLeft);
	InputComponent->BindKey(EKeys::Left, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveLeft);

	InputComponent->BindKey(EKeys::D, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveRight);
	InputComponent->BindKey(EKeys::Right, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::MoveRight);

	InputComponent->BindKey(EKeys::R, EInputEvent::IE_Pressed, this, &APushBoxPlayerController::RestartLevel);
}

void APushBoxPlayerController::MoveUp()
{
	TryMove(GetCameraForwardGridDirection());
}

void APushBoxPlayerController::MoveDown()
{
	const FIntPoint ForwardDir = GetCameraForwardGridDirection();
	TryMove(FIntPoint(-ForwardDir.X, -ForwardDir.Y));
}

void APushBoxPlayerController::MoveLeft()
{
	const FIntPoint RightDir = GetCameraRightGridDirection();
	TryMove(FIntPoint(-RightDir.X, -RightDir.Y));
}

void APushBoxPlayerController::MoveRight()
{
	TryMove(GetCameraRightGridDirection());
}

void APushBoxPlayerController::RestartLevel()
{
	if (ALevelProcessController* ProcessController = ResolveProcessController())
	{
		ProcessController->RestartCurrentLevel();
	}
}

bool APushBoxPlayerController::TryMove(const FIntPoint& Direction)
{
	APushBoxLevelRuntime* LevelRuntime = ResolveLevelRuntime();
	return LevelRuntime ? LevelRuntime->TryMove(Direction) : false;
}

APushBoxLevelRuntime* APushBoxPlayerController::ResolveLevelRuntime() const
{
	if (CachedLevelRuntime)
	{
		return CachedLevelRuntime;
	}

	const ALevelProcessController* ProcessController = ResolveProcessController();
	CachedLevelRuntime = ProcessController ? ProcessController->GetLevelRuntime() : nullptr;
	return CachedLevelRuntime;
}

FIntPoint APushBoxPlayerController::GetCameraForwardGridDirection() const
{
	const FVector CameraForward = PlayerCameraManager ? PlayerCameraManager->GetCameraRotation().Vector() : FVector::ForwardVector;
	return ToCardinalGridDirection(CameraForward);
}

FIntPoint APushBoxPlayerController::GetCameraRightGridDirection() const
{
	const FVector CameraRight = PlayerCameraManager ? FRotationMatrix(PlayerCameraManager->GetCameraRotation()).GetScaledAxis(EAxis::Y) : FVector::RightVector;
	return ToCardinalGridDirection(CameraRight);
}

FIntPoint APushBoxPlayerController::ToCardinalGridDirection(const FVector& WorldDirection)
{
	FVector2D FlatDirection(WorldDirection.X, WorldDirection.Y);
	if (!FlatDirection.Normalize())
	{
		return FIntPoint(1, 0);
	}

	if (FMath::Abs(FlatDirection.X) >= FMath::Abs(FlatDirection.Y))
	{
		return FIntPoint(FlatDirection.X >= 0.0f ? 1 : -1, 0);
	}

	return FIntPoint(0, FlatDirection.Y >= 0.0f ? 1 : -1);
}

ALevelProcessController* APushBoxPlayerController::ResolveProcessController() const
{
	if (CachedProcessController)
	{
		return CachedProcessController;
	}

	CachedProcessController = GetWorld() ? Cast<ALevelProcessController>(UGameplayStatics::GetActorOfClass(GetWorld(), ALevelProcessController::StaticClass())) : nullptr;
	return CachedProcessController;
}
