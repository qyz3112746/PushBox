// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelRuntime.h"

#include "BoxActor.h"
#include "BoxCell.h"
#include "BoxTargetCell.h"
#include "GridCellBase.h"
#include "PlayerSpawnCell.h"
#include "PushBox.h"
#include "PushBoxLevelData.h"
#include "LevelProcessController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

APushBoxLevelRuntime::APushBoxLevelRuntime()
{
	PrimaryActorTick.bCanEverTick = true;
	CellSize = 100.0f;
	VisualBaseCellSize = 100.0f;
	GridOrigin = FVector::ZeroVector;
	InputInterval = 0.1f;
	MoveDuration = 0.1f;
	CurrentLevelData = nullptr;
	ActiveProcessController = nullptr;
	PlayerCoord = FIntPoint::ZeroValue;
	bHasAnnouncedVictory = false;
	NextMoveAllowedTime = 0.0;
	bIsPlayerInterpolatingMove = false;
	PlayerMoveStartLocation = FVector::ZeroVector;
	PlayerMoveTargetLocation = FVector::ZeroVector;
	PlayerMoveStartTime = 0.0f;
	PlayerMoveDuration = 0.0f;
	bHasPlayerLockedZ = false;
	PlayerLockedZ = 0.0f;
}

void APushBoxLevelRuntime::BeginPlay()
{
	Super::BeginPlay();
}

void APushBoxLevelRuntime::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsPlayerInterpolatingMove)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn)
	{
		bIsPlayerInterpolatingMove = false;
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || PlayerMoveDuration <= KINDA_SMALL_NUMBER)
	{
		PlayerPawn->SetActorLocation(PlayerMoveTargetLocation);
		bIsPlayerInterpolatingMove = false;
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - PlayerMoveStartTime;
	const float Alpha = FMath::Clamp(Elapsed / PlayerMoveDuration, 0.0f, 1.0f);
	PlayerPawn->SetActorLocation(FMath::Lerp(PlayerMoveStartLocation, PlayerMoveTargetLocation, Alpha));
	if (Alpha >= 1.0f)
	{
		bIsPlayerInterpolatingMove = false;
	}
}

bool APushBoxLevelRuntime::LoadLevel(UPushBoxLevelData* InLevelData)
{
	if (!InLevelData)
	{
		UE_LOG(LogPushBox, Error, TEXT("LoadLevel failed: LevelData is null."));
		return false;
	}

	ClearSpawnedLevel();
	CurrentLevelData = InLevelData;
	CurrentLevelData->NormalizeGrid(CurrentLevelData->DefaultCellClass);
	bHasAnnouncedVictory = false;
	PlayerCoord = FIntPoint::ZeroValue;
	bool bFoundPlayerSpawn = false;
	for (int32 Y = 0; Y < InLevelData->GridHeight; ++Y)
	{
		for (int32 X = 0; X < InLevelData->GridWidth; ++X)
		{
			const FIntPoint GridCoord(X, Y);
			TSubclassOf<AGridCellBase> CellClass = InLevelData->GetCellAt(GridCoord);
			if (!CellClass)
			{
				CellClass = InLevelData->DefaultCellClass;
			}

			if (!CellClass)
			{
				UE_LOG(LogPushBox, Warning, TEXT("Skip empty cell at (%d, %d): both GridRows and DefaultCellClass are null."), GridCoord.X, GridCoord.Y);
				continue;
			}

			if (!bFoundPlayerSpawn && CellClass->IsChildOf(APlayerSpawnCell::StaticClass()))
			{
				PlayerCoord = GridCoord;
				bFoundPlayerSpawn = true;
			}

			if (!SpawnCell(CellClass, GridCoord))
			{
				UE_LOG(LogPushBox, Error, TEXT("Failed to spawn cell at (%d, %d)."), GridCoord.X, GridCoord.Y);
				ClearSpawnedLevel();
				CurrentLevelData = nullptr;
				return false;
			}
		}
	}

	RefreshTargetMatchedEffects();
	bIsPlayerInterpolatingMove = false;
	NextMoveAllowedTime = 0.0;
	bHasPlayerLockedZ = false;
	PlayerLockedZ = 0.0f;
	MovePlayerToCoord(PlayerCoord, true);
	return true;
}

bool APushBoxLevelRuntime::TryMove(const FIntPoint& Direction)
{
	if (!CurrentLevelData)
	{
		return false;
	}

	if (const UWorld* World = GetWorld())
	{
		if (World->GetTimeSeconds() < NextMoveAllowedTime)
		{
			return false;
		}
	}

	if (Direction == FIntPoint::ZeroValue)
	{
		return false;
	}

	const FIntPoint TargetCoord = PlayerCoord + Direction;
	if (!IsWithinGrid(TargetCoord))
	{
		return false;
	}

	const FCellMoveContext PlayerMoveContext{
		ECellMoverType::Player,
		Direction,
		PlayerCoord,
		TargetCoord
	};

	if (!CanMoverExitCell(ECellMoverType::Player, PlayerMoveContext))
	{
		return false;
	}

	if (TObjectPtr<ABoxActor>* BoxAtTarget = BoxByCoord.Find(TargetCoord))
	{
		const FIntPoint BoxDestination = TargetCoord + Direction;
		if (!IsWithinGrid(BoxDestination) || BlockingCoords.Contains(BoxDestination) || BoxByCoord.Contains(BoxDestination))
		{
			return false;
		}

		const FCellMoveContext BoxMoveContext{
			ECellMoverType::Box,
			Direction,
			TargetCoord,
			BoxDestination
		};

		if (!CanMoverExitCell(ECellMoverType::Box, BoxMoveContext))
		{
			return false;
		}

		if (!CanMoverEnterCell(ECellMoverType::Box, BoxMoveContext))
		{
			return false;
		}

		if (!CanMoverEnterCell(ECellMoverType::Player, PlayerMoveContext))
		{
			return false;
		}

		ABoxActor* BoxActor = BoxAtTarget->Get();
		BoxByCoord.Remove(TargetCoord);
		BoxByCoord.Add(BoxDestination, BoxActor);
		BoxCoordByActor.Add(BoxActor, BoxDestination);

		BoxActor->MoveToGridCoord(BoxDestination, GridToWorld(BoxDestination), MoveDuration);

		NotifyMoverExitCell(ECellMoverType::Box, BoxMoveContext);
		NotifyMoverEnterCell(ECellMoverType::Box, BoxMoveContext);
	}
	else if (BlockingCoords.Contains(TargetCoord))
	{
		return false;
	}
	else if (!CanMoverEnterCell(ECellMoverType::Player, PlayerMoveContext))
	{
		return false;
	}

	PlayerCoord = TargetCoord;
	MovePlayerToCoord(PlayerCoord, false);
	NotifyMoverExitCell(ECellMoverType::Player, PlayerMoveContext);
	NotifyMoverEnterCell(ECellMoverType::Player, PlayerMoveContext);
	RefreshTargetMatchedEffects();

	if (const UWorld* World = GetWorld())
	{
		NextMoveAllowedTime = World->GetTimeSeconds() + FMath::Max(InputInterval, 0.0f);
	}

	if (!bHasAnnouncedVictory && CheckVictory())
	{
		bHasAnnouncedVictory = true;
		OnLevelFinished.Broadcast(true);
	}

	return true;
}

bool APushBoxLevelRuntime::CheckVictory() const
{
	if (TargetByCoord.Num() == 0)
	{
		return false;
	}

	for (const TPair<FIntPoint, TObjectPtr<ABoxTargetCell>>& Pair : TargetByCoord)
	{
		const ABoxTargetCell* TargetCell = Pair.Value.Get();
		if (!TargetCell)
		{
			return false;
		}

		const TObjectPtr<ABoxActor>* BoxPtr = BoxByCoord.Find(Pair.Key);
		if (!BoxPtr)
		{
			return false;
		}

		const ABoxActor* BoxActor = BoxPtr->Get();
		if (!BoxActor)
		{
			return false;
		}

		if (!TargetCell->IsBoxAccepted(BoxActor))
		{
			return false;
		}
	}

	return true;
}

bool APushBoxLevelRuntime::ResetToInitialState()
{
	return CurrentLevelData ? LoadLevel(CurrentLevelData) : false;
}

FName APushBoxLevelRuntime::GetCurrentLevelId() const
{
	return CurrentLevelData ? CurrentLevelData->LevelId : NAME_None;
}

void APushBoxLevelRuntime::SetActiveProcessController(ALevelProcessController* InController)
{
	ActiveProcessController = InController;
}

ABoxActor* APushBoxLevelRuntime::RegisterSpawnedBox(const FIntPoint& SpawnCoord, TSubclassOf<ABoxActor> BoxClass, const FIntPoint& TargetCoord)
{
	if (!CurrentLevelData)
	{
		return nullptr;
	}

	if (!IsWithinGrid(SpawnCoord))
	{
		return nullptr;
	}

	if (BoxByCoord.Contains(SpawnCoord))
	{
		return nullptr;
	}

	TSubclassOf<ABoxActor> SpawnClass = BoxClass;
	if (!SpawnClass)
	{
		SpawnClass = ABoxActor::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABoxActor* BoxActor = GetWorld()->SpawnActor<ABoxActor>(SpawnClass, GridToWorld(SpawnCoord), FRotator::ZeroRotator, SpawnParams);
	if (!BoxActor)
	{
		return nullptr;
	}

	const float SafeBaseSize = FMath::Max(VisualBaseCellSize, 1.0f);
	const float UniformScale = CellSize / SafeBaseSize;
	BoxActor->SetActorScale3D(FVector(UniformScale));
	BoxActor->SetGridCoord(SpawnCoord);
	BoxActor->SetTargetCoord(TargetCoord);

	LiveBoxes.Add(BoxActor);
	BoxByCoord.Add(SpawnCoord, BoxActor);
	BoxCoordByActor.Add(BoxActor, SpawnCoord);
	return BoxActor;
}

void APushBoxLevelRuntime::ClearSpawnedLevel()
{
	for (AGridCellBase* SpawnedCell : SpawnedCells)
	{
		if (IsValid(SpawnedCell))
		{
			SpawnedCell->Destroy();
		}
	}

	for (ABoxActor* SpawnedBox : LiveBoxes)
	{
		if (IsValid(SpawnedBox))
		{
			SpawnedBox->Destroy();
		}
	}

	SpawnedCells.Reset();
	LiveBoxes.Reset();
	BoxByCoord.Reset();
	BoxCoordByActor.Reset();
	TargetByCoord.Reset();
	BlockingCoords.Reset();
}

AGridCellBase* APushBoxLevelRuntime::SpawnCell(const TSubclassOf<AGridCellBase>& CellClass, const FIntPoint& GridCoord)
{
	if (!CellClass)
	{
		return nullptr;
	}

	const FVector SpawnLocation = GridToWorld(GridCoord);
	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);
	AGridCellBase* SpawnedCell = GetWorld()->SpawnActorDeferred<AGridCellBase>(
		CellClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);
	if (!SpawnedCell)
	{
		return nullptr;
	}

	const float SafeBaseSize = FMath::Max(VisualBaseCellSize, 1.0f);
	const float UniformScale = CellSize / SafeBaseSize;
	SpawnedCell->SetActorScale3D(FVector(UniformScale));
	SpawnedCell->SetGridCoord(GridCoord);
	UGameplayStatics::FinishSpawningActor(SpawnedCell, SpawnTransform);
	RegisterSpawnedCell(SpawnedCell);
	return SpawnedCell;
}

void APushBoxLevelRuntime::RegisterSpawnedCell(AGridCellBase* SpawnedCell)
{
	if (!SpawnedCell)
	{
		return;
	}

	const FIntPoint Coord = SpawnedCell->GetGridCoord();
	SpawnedCells.Add(SpawnedCell);

	if (SpawnedCell->IsA(ABoxCell::StaticClass()))
	{
		return;
	}

	if (ABoxTargetCell* TargetCell = Cast<ABoxTargetCell>(SpawnedCell))
	{
		TargetByCoord.Add(Coord, TargetCell);
	}

	if (SpawnedCell->IsBlockingMovement())
	{
		BlockingCoords.Add(Coord);
	}
}

void APushBoxLevelRuntime::UnregisterSpawnedCell(AGridCellBase* SpawnedCell)
{
	if (!SpawnedCell)
	{
		return;
	}

	const FIntPoint Coord = SpawnedCell->GetGridCoord();
	SpawnedCells.RemoveSingleSwap(SpawnedCell);
	BlockingCoords.Remove(Coord);

	if (ABoxTargetCell* ExistingTarget = Cast<ABoxTargetCell>(SpawnedCell))
	{
		const TObjectPtr<ABoxTargetCell>* FoundTarget = TargetByCoord.Find(Coord);
		if (FoundTarget && FoundTarget->Get() == ExistingTarget)
		{
			TargetByCoord.Remove(Coord);
		}
	}
}

void APushBoxLevelRuntime::RefreshTargetMatchedEffects()
{
	for (const TPair<FIntPoint, TObjectPtr<ABoxTargetCell>>& Pair : TargetByCoord)
	{
		ABoxTargetCell* TargetCell = Pair.Value.Get();
		if (!TargetCell)
		{
			continue;
		}

		bool bMatched = false;
		if (const TObjectPtr<ABoxActor>* BoxAtTarget = BoxByCoord.Find(Pair.Key))
		{
			const ABoxActor* BoxActor = BoxAtTarget->Get();
			bMatched = BoxActor && TargetCell->IsBoxAccepted(BoxActor);
		}

		TargetCell->SetMatchedState(bMatched);
	}
}

AGridCellBase* APushBoxLevelRuntime::FindSpawnedCellAt(const FIntPoint& GridCoord) const
{
	for (AGridCellBase* SpawnedCell : SpawnedCells)
	{
		if (SpawnedCell && SpawnedCell->GetGridCoord() == GridCoord)
		{
			return SpawnedCell;
		}
	}

	return nullptr;
}

TArray<AGridCellBase*> APushBoxLevelRuntime::FindAllCellsAt(const FIntPoint& GridCoord) const
{
	TArray<AGridCellBase*> Cells;
	for (AGridCellBase* SpawnedCell : SpawnedCells)
	{
		if (SpawnedCell && SpawnedCell->GetGridCoord() == GridCoord)
		{
			Cells.Add(SpawnedCell);
		}
	}
	return Cells;
}

bool APushBoxLevelRuntime::CanMoverExitCell(ECellMoverType MoverType, const FCellMoveContext& Context) const
{
	for (AGridCellBase* Cell : FindAllCellsAt(Context.FromCoord))
	{
		if (!Cell)
		{
			continue;
		}

		const bool bAllowed = (MoverType == ECellMoverType::Player)
			? Cell->CanPlayerExit(Context)
			: Cell->CanBoxExit(Context);
		if (!bAllowed)
		{
			return false;
		}
	}

	return true;
}

bool APushBoxLevelRuntime::CanMoverEnterCell(ECellMoverType MoverType, const FCellMoveContext& Context) const
{
	for (AGridCellBase* Cell : FindAllCellsAt(Context.ToCoord))
	{
		if (!Cell)
		{
			continue;
		}

		const bool bAllowed = (MoverType == ECellMoverType::Player)
			? Cell->CanPlayerEnter(Context)
			: Cell->CanBoxEnter(Context);
		if (!bAllowed)
		{
			return false;
		}
	}

	return true;
}

void APushBoxLevelRuntime::NotifyMoverExitCell(ECellMoverType MoverType, const FCellMoveContext& Context)
{
	for (AGridCellBase* Cell : FindAllCellsAt(Context.FromCoord))
	{
		if (!Cell)
		{
			continue;
		}

		if (MoverType == ECellMoverType::Player)
		{
			Cell->OnPlayerExit(Context);
		}
		else
		{
			Cell->OnBoxExit(Context);
		}
	}
}

void APushBoxLevelRuntime::NotifyMoverEnterCell(ECellMoverType MoverType, const FCellMoveContext& Context)
{
	for (AGridCellBase* Cell : FindAllCellsAt(Context.ToCoord))
	{
		if (!Cell)
		{
			continue;
		}

		if (MoverType == ECellMoverType::Player)
		{
			Cell->OnPlayerEnter(Context);
		}
		else
		{
			Cell->OnBoxEnter(Context);
		}
	}
}

bool APushBoxLevelRuntime::IsWithinGrid(const FIntPoint& GridCoord) const
{
	if (!CurrentLevelData)
	{
		return false;
	}

	return GridCoord.X >= 0 &&
		GridCoord.Y >= 0 &&
		GridCoord.X < CurrentLevelData->GridWidth &&
		GridCoord.Y < CurrentLevelData->GridHeight;
}

FVector APushBoxLevelRuntime::GridToWorld(const FIntPoint& GridCoord) const
{
	return GridOrigin + FVector(GridCoord.X * CellSize, GridCoord.Y * CellSize, 0.0f);
}

void APushBoxLevelRuntime::MovePlayerToCoord(const FIntPoint& GridCoord, bool bInstant)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn)
	{
		return;
	}

	if (!bHasPlayerLockedZ)
	{
		PlayerLockedZ = PlayerPawn->GetActorLocation().Z;
		bHasPlayerLockedZ = true;
	}

	const FVector TargetLocation = GridToWorld(GridCoord);
	const FVector LockedTargetLocation(TargetLocation.X, TargetLocation.Y, PlayerLockedZ);
	if (bInstant || MoveDuration <= KINDA_SMALL_NUMBER || !GetWorld())
	{
		PlayerPawn->SetActorLocation(LockedTargetLocation);
		bIsPlayerInterpolatingMove = false;
		return;
	}

	PlayerMoveStartLocation = PlayerPawn->GetActorLocation();
	PlayerMoveTargetLocation = LockedTargetLocation;
	PlayerMoveStartTime = GetWorld()->GetTimeSeconds();
	PlayerMoveDuration = MoveDuration;
	bIsPlayerInterpolatingMove = true;
}
