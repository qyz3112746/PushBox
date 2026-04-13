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
#include "Containers/Queue.h"

APushBoxLevelRuntime::APushBoxLevelRuntime()
{
	PrimaryActorTick.bCanEverTick = true;
	CellSize = 100.0f;
	VisualBaseCellSize = 100.0f;
	GridOrigin = FVector::ZeroVector;
	InputInterval = 0.1f;
	MoveDuration = 0.1f;
	CurrentLevelData = nullptr;
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
	TMap<UClass*, int32> AvailableBoxCounts;
	TArray<TArray<UClass*>> TargetRequirements;

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

			const ABoxTargetCell* TargetCDO = Cast<ABoxTargetCell>(CellDef.CellClass->GetDefaultObject());
			if (!TargetCDO || TargetCDO->RequiredBoxActorTypes.Num() == 0)
			{
				UE_LOG(LogPushBox, Error, TEXT("Target cell class '%s' must define RequiredBoxActorTypes."), *CellDef.CellClass->GetName());
				return false;
			}

			TArray<UClass*> RequiredTypes;
			for (const TSubclassOf<ABoxActor>& RequiredType : TargetCDO->RequiredBoxActorTypes)
			{
				UClass* RequiredClass = RequiredType.Get();
				if (!RequiredClass)
				{
					UE_LOG(LogPushBox, Error, TEXT("Target cell class '%s' contains an empty required box type."), *CellDef.CellClass->GetName());
					return false;
				}
				RequiredTypes.Add(RequiredClass);
			}
			TargetRequirements.Add(MoveTemp(RequiredTypes));
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

			UClass* SpawnBoxClass = BoxCDO->BoxSpawnClass.Get();
			if (!SpawnBoxClass)
			{
				UE_LOG(LogPushBox, Error, TEXT("Box cell class '%s' must define BoxSpawnClass."), *CellDef.CellClass->GetName());
				return false;
			}

			AvailableBoxCounts.FindOrAdd(SpawnBoxClass)++;
		}
	}

	if (SpawnCount != 1)
	{
		UE_LOG(LogPushBox, Error, TEXT("Level '%s' must contain exactly one PlayerSpawnCell (found %d)."),
			*InLevelData->GetName(), SpawnCount);
		return false;
	}

	if (BoxCount < TargetCount)
	{
		UE_LOG(LogPushBox, Error, TEXT("Level '%s' must contain enough boxes for all targets (boxes=%d, targets=%d)."),
			*InLevelData->GetName(), BoxCount, TargetCount);
		return false;
	}

	// Build a small max-flow (target instances -> box classes with supply) to avoid exponential backtracking.
	TMap<UClass*, int32> ClassToIndex;
	TArray<UClass*> IndexedClasses;
	TArray<int32> ClassSupply;
	for (const TPair<UClass*, int32>& Pair : AvailableBoxCounts)
	{
		const int32 NewIndex = IndexedClasses.Add(Pair.Key);
		ClassToIndex.Add(Pair.Key, NewIndex);
		ClassSupply.Add(Pair.Value);
	}

	for (const TArray<UClass*>& RequiredTypes : TargetRequirements)
	{
		bool bHasAnyAvailableType = false;
		for (UClass* RequiredType : RequiredTypes)
		{
			if (const int32* ClassIndex = ClassToIndex.Find(RequiredType))
			{
				if (ClassSupply[*ClassIndex] > 0)
				{
					bHasAnyAvailableType = true;
					break;
				}
			}
		}
		if (!bHasAnyAvailableType)
		{
			UE_LOG(LogPushBox, Error, TEXT("Level '%s' has a target requiring unavailable box classes."), *InLevelData->GetName());
			return false;
		}
	}

	struct FFlowEdge
	{
		int32 To = 0;
		int32 Rev = 0;
		int32 Cap = 0;
	};

	auto AddEdge = [](TArray<TArray<FFlowEdge>>& Graph, int32 From, int32 To, int32 Cap)
	{
		const int32 FwdIndex = Graph[From].Num();
		const int32 RevIndex = Graph[To].Num();
		Graph[From].Add({To, RevIndex, Cap});
		Graph[To].Add({From, FwdIndex, 0});
	};

	const int32 TargetNodeOffset = 1;
	const int32 ClassNodeOffset = TargetNodeOffset + TargetRequirements.Num();
	const int32 SinkNode = ClassNodeOffset + IndexedClasses.Num();
	const int32 NodeCount = SinkNode + 1;
	TArray<TArray<FFlowEdge>> Graph;
	Graph.SetNum(NodeCount);

	for (int32 TargetIdx = 0; TargetIdx < TargetRequirements.Num(); ++TargetIdx)
	{
		const int32 TargetNode = TargetNodeOffset + TargetIdx;
		AddEdge(Graph, 0, TargetNode, 1);

		for (UClass* RequiredType : TargetRequirements[TargetIdx])
		{
			const int32* ClassIndex = ClassToIndex.Find(RequiredType);
			if (!ClassIndex)
			{
				continue;
			}

			const int32 ClassNode = ClassNodeOffset + *ClassIndex;
			AddEdge(Graph, TargetNode, ClassNode, 1);
		}
	}

	for (int32 ClassIdx = 0; ClassIdx < ClassSupply.Num(); ++ClassIdx)
	{
		const int32 ClassNode = ClassNodeOffset + ClassIdx;
		AddEdge(Graph, ClassNode, SinkNode, ClassSupply[ClassIdx]);
	}

	auto BuildLevelGraph = [](const TArray<TArray<FFlowEdge>>& GraphRef, int32 Source, TArray<int32>& OutLevel) -> bool
	{
		OutLevel.Init(-1, GraphRef.Num());
		TQueue<int32> Queue;
		OutLevel[Source] = 0;
		Queue.Enqueue(Source);

		while (!Queue.IsEmpty())
		{
			int32 Node = 0;
			Queue.Dequeue(Node);
			for (const FFlowEdge& Edge : GraphRef[Node])
			{
				if (Edge.Cap > 0 && OutLevel[Edge.To] < 0)
				{
					OutLevel[Edge.To] = OutLevel[Node] + 1;
					Queue.Enqueue(Edge.To);
				}
			}
		}

		return OutLevel.Last() >= 0;
	};

	TFunction<int32(int32, int32, TArray<int32>&, const TArray<int32>&)> SendFlow;
	SendFlow = [&](int32 Node, int32 Flow, TArray<int32>& It, const TArray<int32>& Level) -> int32
	{
		if (Node == SinkNode)
		{
			return Flow;
		}

		for (; It[Node] < Graph[Node].Num(); ++It[Node])
		{
			FFlowEdge& Edge = Graph[Node][It[Node]];
			if (Edge.Cap <= 0 || Level[Edge.To] != Level[Node] + 1)
			{
				continue;
			}

			const int32 Pushed = SendFlow(Edge.To, FMath::Min(Flow, Edge.Cap), It, Level);
			if (Pushed > 0)
			{
				Edge.Cap -= Pushed;
				Graph[Edge.To][Edge.Rev].Cap += Pushed;
				return Pushed;
			}
		}

		return 0;
	};

	int32 MaxFlow = 0;
	TArray<int32> Level;
	while (BuildLevelGraph(Graph, 0, Level))
	{
		TArray<int32> It;
		It.Init(0, Graph.Num());
		while (true)
		{
			const int32 Pushed = SendFlow(0, TNumericLimits<int32>::Max(), It, Level);
			if (Pushed <= 0)
			{
				break;
			}
			MaxFlow += Pushed;
		}
	}

	if (MaxFlow < TargetRequirements.Num())
	{
		UE_LOG(LogPushBox, Error, TEXT("Level '%s' does not have enough strict-matching box classes to satisfy all targets."), *InLevelData->GetName());
		return false;
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
