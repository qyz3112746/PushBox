// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxLevelRuntime.h"

#include "BoxActor.h"
#include "BoxCell.h"
#include "BoxTargetCell.h"
#include "GridCellBase.h"
#include "PlayerSpawnCell.h"
#include "PushBox.h"
#include "PushBoxLevelData.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

APushBoxLevelRuntime::APushBoxLevelRuntime()
{
	PrimaryActorTick.bCanEverTick = false;
	CellSize = 100.0f;
	VisualBaseCellSize = 100.0f;
	GridOrigin = FVector::ZeroVector;
	CurrentLevelData = nullptr;
	PlayerCoord = FIntPoint::ZeroValue;
	bHasAnnouncedVictory = false;
}

void APushBoxLevelRuntime::BeginPlay()
{
	Super::BeginPlay();
}

bool APushBoxLevelRuntime::LoadLevel(UPushBoxLevelData* InLevelData)
{
	if (!InLevelData)
	{
		UE_LOG(LogPushBox, Error, TEXT("LoadLevel failed: LevelData is null."));
		return false;
	}

	FLevelValidationResult Validation;
	if (!ValidateLevelData(InLevelData, Validation))
	{
		return false;
	}

	ClearSpawnedLevel();
	CurrentLevelData = InLevelData;
	bHasAnnouncedVictory = false;
	PlayerCoord = Validation.PlayerSpawnCoord;

	if (InLevelData->DefaultCellClass)
	{
		for (int32 X = 0; X < InLevelData->GridWidth; ++X)
		{
			for (int32 Y = 0; Y < InLevelData->GridHeight; ++Y)
			{
				SpawnCell(InLevelData->DefaultCellClass, FIntPoint(X, Y));
			}
		}
	}

	for (const FPushBoxCellSpawnData& CellDef : InLevelData->CellDefinitions)
	{
		if (AGridCellBase* ExistingCell = FindSpawnedCellAt(CellDef.GridCoord))
		{
			UnregisterSpawnedCell(ExistingCell);
			ExistingCell->Destroy();
		}

		if (!SpawnCell(CellDef.CellClass, CellDef.GridCoord))
		{
			UE_LOG(LogPushBox, Error, TEXT("Failed to spawn cell at (%d, %d)."), CellDef.GridCoord.X, CellDef.GridCoord.Y);
			ClearSpawnedLevel();
			CurrentLevelData = nullptr;
			return false;
		}
	}

	for (AGridCellBase* SpawnedCell : SpawnedCells)
	{
		if (ABoxCell* BoxCell = Cast<ABoxCell>(SpawnedCell))
		{
			if (!SpawnBoxFromCell(BoxCell))
			{
				UE_LOG(LogPushBox, Error, TEXT("Failed to spawn box actor at (%d, %d)."), BoxCell->GetGridCoord().X, BoxCell->GetGridCoord().Y);
				ClearSpawnedLevel();
				CurrentLevelData = nullptr;
				return false;
			}
		}
	}

	MovePlayerToCoord(PlayerCoord);
	return true;
}

bool APushBoxLevelRuntime::TryMove(const FIntPoint& Direction)
{
	if (!CurrentLevelData)
	{
		return false;
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

	if (TObjectPtr<ABoxActor>* BoxAtTarget = BoxByCoord.Find(TargetCoord))
	{
		const FIntPoint BoxDestination = TargetCoord + Direction;
		if (!IsWithinGrid(BoxDestination) || BlockingCoords.Contains(BoxDestination) || BoxByCoord.Contains(BoxDestination))
		{
			return false;
		}

		ABoxActor* BoxActor = BoxAtTarget->Get();
		BoxByCoord.Remove(TargetCoord);
		BoxByCoord.Add(BoxDestination, BoxActor);

		BoxActor->MoveToGridCoord(BoxDestination, GridToWorld(BoxDestination));
	}
	else if (BlockingCoords.Contains(TargetCoord))
	{
		return false;
	}

	PlayerCoord = TargetCoord;
	MovePlayerToCoord(PlayerCoord);

	if (!bHasAnnouncedVictory && CheckVictory())
	{
		bHasAnnouncedVictory = true;
		OnLevelFinished.Broadcast(true);
	}

	return true;
}

bool APushBoxLevelRuntime::CheckVictory() const
{
	if (BoxByCoord.Num() == 0)
	{
		return false;
	}

	for (const TPair<FIntPoint, TObjectPtr<ABoxActor>>& Pair : BoxByCoord)
	{
		const TObjectPtr<ABoxTargetCell>* TargetPtr = TargetByCoord.Find(Pair.Key);
		if (!TargetPtr)
		{
			return false;
		}

		const ABoxActor* BoxActor = Pair.Value.Get();
		const ABoxTargetCell* TargetCell = TargetPtr->Get();
		if (!BoxActor || !TargetCell)
		{
			return false;
		}

		if (BoxActor->RequiredTargetCellClass && !TargetCell->IsA(BoxActor->RequiredTargetCellClass))
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

bool APushBoxLevelRuntime::ValidateLevelData(const UPushBoxLevelData* InLevelData, FLevelValidationResult& OutValidation) const
{
	OutValidation = FLevelValidationResult();

	if (!InLevelData)
	{
		UE_LOG(LogPushBox, Error, TEXT("ValidateLevelData failed: LevelData is null."));
		return false;
	}

	if (InLevelData->GridWidth <= 0 || InLevelData->GridHeight <= 0)
	{
		UE_LOG(LogPushBox, Error, TEXT("Level '%s' has invalid grid size."), *InLevelData->GetName());
		return false;
	}

	TSet<FIntPoint> OccupiedCoords;
	int32 SpawnCount = 0;
	int32 BoxCount = 0;
	int32 TargetCount = 0;
	TMap<TSubclassOf<ABoxTargetCell>, int32> BoxCountsByTargetClass;

	for (const FPushBoxCellSpawnData& CellDef : InLevelData->CellDefinitions)
	{
		if (!CellDef.CellClass)
		{
			UE_LOG(LogPushBox, Error, TEXT("Level '%s' has an empty CellClass in CellDefinitions."), *InLevelData->GetName());
			return false;
		}

		if (CellDef.GridCoord.X < 0 || CellDef.GridCoord.X >= InLevelData->GridWidth ||
			CellDef.GridCoord.Y < 0 || CellDef.GridCoord.Y >= InLevelData->GridHeight)
		{
			UE_LOG(LogPushBox, Error, TEXT("Cell (%d, %d) is out of bounds in level '%s'."),
				CellDef.GridCoord.X, CellDef.GridCoord.Y, *InLevelData->GetName());
			return false;
		}

		if (OccupiedCoords.Contains(CellDef.GridCoord))
		{
			UE_LOG(LogPushBox, Error, TEXT("Duplicate cell definition at (%d, %d) in level '%s'."),
				CellDef.GridCoord.X, CellDef.GridCoord.Y, *InLevelData->GetName());
			return false;
		}
		OccupiedCoords.Add(CellDef.GridCoord);

		if (CellDef.CellClass->IsChildOf(APlayerSpawnCell::StaticClass()))
		{
			++SpawnCount;
			OutValidation.PlayerSpawnCoord = CellDef.GridCoord;
			continue;
		}

		if (CellDef.CellClass->IsChildOf(ABoxTargetCell::StaticClass()))
		{
			++TargetCount;
			const TSubclassOf<ABoxTargetCell> TargetClass = CellDef.CellClass.Get();
			OutValidation.TargetCountsByType.FindOrAdd(TargetClass)++;
			continue;
		}

		if (CellDef.CellClass->IsChildOf(ABoxCell::StaticClass()))
		{
			++BoxCount;

			const ABoxCell* BoxCDO = Cast<ABoxCell>(CellDef.CellClass->GetDefaultObject());
			if (!BoxCDO)
			{
				UE_LOG(LogPushBox, Error, TEXT("Invalid box class setup in level '%s'."), *InLevelData->GetName());
				return false;
			}

			if (!BoxCDO->TargetCellClass)
			{
				UE_LOG(LogPushBox, Error, TEXT("Box class '%s' must define TargetCellClass."), *CellDef.CellClass->GetName());
				return false;
			}

			BoxCountsByTargetClass.FindOrAdd(BoxCDO->TargetCellClass)++;
		}
	}

	if (SpawnCount != 1)
	{
		UE_LOG(LogPushBox, Error, TEXT("Level '%s' must contain exactly one PlayerSpawnCell (found %d)."),
			*InLevelData->GetName(), SpawnCount);
		return false;
	}

	if (BoxCount != TargetCount)
	{
		UE_LOG(LogPushBox, Error, TEXT("Level '%s' must contain equal box and target counts (boxes=%d, targets=%d)."),
			*InLevelData->GetName(), BoxCount, TargetCount);
		return false;
	}

	for (const TPair<TSubclassOf<ABoxTargetCell>, int32>& BoxRequirement : BoxCountsByTargetClass)
	{
		int32 AvailableTargets = 0;
		for (const TPair<TSubclassOf<ABoxTargetCell>, int32>& TargetPair : OutValidation.TargetCountsByType)
		{
			UClass* RequiredClass = BoxRequirement.Key.Get();
			UClass* ExistingTargetClass = TargetPair.Key.Get();
			if (RequiredClass && ExistingTargetClass && ExistingTargetClass->IsChildOf(RequiredClass))
			{
				AvailableTargets += TargetPair.Value;
			}
		}

		if (AvailableTargets == 0)
		{
			UE_LOG(LogPushBox, Error, TEXT("No target cell found for required target class '%s' in level '%s'."),
				*GetNameSafe(BoxRequirement.Key.Get()), *InLevelData->GetName());
			return false;
		}

		if (BoxRequirement.Value > AvailableTargets)
		{
			UE_LOG(LogPushBox, Error, TEXT("Not enough target cells for class '%s' in level '%s' (boxes=%d, targets=%d)."),
				*GetNameSafe(BoxRequirement.Key.Get()), *InLevelData->GetName(), BoxRequirement.Value, AvailableTargets);
			return false;
		}
	}

	OutValidation.bIsValid = true;
	return true;
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

	for (ABoxActor* SpawnedBox : SpawnedBoxes)
	{
		if (IsValid(SpawnedBox))
		{
			SpawnedBox->Destroy();
		}
	}

	SpawnedCells.Reset();
	SpawnedBoxes.Reset();
	BoxByCoord.Reset();
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
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGridCellBase* SpawnedCell = GetWorld()->SpawnActor<AGridCellBase>(CellClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!SpawnedCell)
	{
		return nullptr;
	}

	const float SafeBaseSize = FMath::Max(VisualBaseCellSize, 1.0f);
	const float UniformScale = CellSize / SafeBaseSize;
	SpawnedCell->SetActorScale3D(FVector(UniformScale));

	SpawnedCell->SetGridCoord(GridCoord);
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

	if (ABoxCell* BoxCell = Cast<ABoxCell>(SpawnedCell))
	{
		return;
	}

	if (ABoxTargetCell* TargetCell = Cast<ABoxTargetCell>(SpawnedCell))
	{
		TargetByCoord.Add(Coord, TargetCell);
	}

	if (SpawnedCell->IsBlockingMovement())
	{
		if (Cast<ABoxCell>(SpawnedCell))
		{
			return;
		}

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

ABoxActor* APushBoxLevelRuntime::SpawnBoxFromCell(ABoxCell* BoxCell)
{
	if (!BoxCell)
	{
		return nullptr;
	}

	TSubclassOf<ABoxActor> BoxClass = BoxCell->BoxActorClass;
	if (!BoxClass)
	{
		BoxClass = ABoxActor::StaticClass();
	}

	const FIntPoint BoxCoord = BoxCell->GetGridCoord();
	const FVector SpawnLocation = GridToWorld(BoxCoord);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABoxActor* BoxActor = GetWorld()->SpawnActor<ABoxActor>(BoxClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!BoxActor)
	{
		return nullptr;
	}

	const float SafeBaseSize = FMath::Max(VisualBaseCellSize, 1.0f);
	const float UniformScale = CellSize / SafeBaseSize;
	BoxActor->SetActorScale3D(FVector(UniformScale));
	BoxActor->SetGridCoord(BoxCoord);
	BoxActor->RequiredTargetCellClass = BoxCell->TargetCellClass;
	BoxActor->SetBoxMesh(BoxCell->BoxActorMesh);
	BoxActor->BoxMeshComponent->SetRelativeTransform(BoxCell->GetCellMeshRelativeTransform());

	SpawnedBoxes.Add(BoxActor);
	BoxByCoord.Add(BoxCoord, BoxActor);
	return BoxActor;
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

void APushBoxLevelRuntime::MovePlayerToCoord(const FIntPoint& GridCoord) const
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn)
	{
		return;
	}

	PlayerPawn->SetActorLocation(GridToWorld(GridCoord));
}
