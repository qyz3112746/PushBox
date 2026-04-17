// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridCellBase.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UStaticMesh;
class ABoxActor;
class APushBoxLevelRuntime;
class APawn;
class UPushBoxLevelData;

UENUM(BlueprintType)
enum class ECellMoverType : uint8
{
	Player,
	Box
};

USTRUCT(BlueprintType)
struct FCellMoveContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	ECellMoverType MoverType = ECellMoverType::Player;

	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	FIntPoint Direction = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	FIntPoint FromCoord = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	FIntPoint ToCoord = FIntPoint::ZeroValue;
};

USTRUCT(BlueprintType)
struct FCellEditorPreviewContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Cell|Preview")
	FVector GridOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Cell|Preview")
	float CellSize = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Cell|Preview")
	float VisualBaseCellSize = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Cell|Preview")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadWrite, Category = "Cell|Preview")
	TObjectPtr<UPushBoxLevelData> LevelData = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Cell|Preview")
	TSubclassOf<APawn> PreviewPawnClass;

	UPROPERTY(BlueprintReadWrite, Category = "Cell|Preview")
	TObjectPtr<AActor> PreviewAttachActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "Cell|Preview")
	TArray<TObjectPtr<AActor>> SpawnedActors;
};

UCLASS(Blueprintable, meta = (PrioritizeCategories = "Cell"))
class PUSHBOX_API AGridCellBase : public AActor
{
	GENERATED_BODY()

public:
	AGridCellBase();

	UFUNCTION(BlueprintCallable, Category = "Cell")
	void SetGridCoord(const FIntPoint& InGridCoord);

	UFUNCTION(BlueprintPure, Category = "Cell")
	FIntPoint GetGridCoord() const { return GridCoord; }

	UFUNCTION(BlueprintPure, Category = "Cell")
	bool IsBlockingMovement() const { return bBlocksMovement; }

	UFUNCTION(BlueprintCallable, Category = "Cell")
	void SetBlocksMovement(bool bInBlocksMovement);

	UFUNCTION(BlueprintPure, Category = "Cell")
	UStaticMesh* GetCellStaticMesh() const;

	UFUNCTION(BlueprintPure, Category = "Cell")
	FTransform GetCellMeshRelativeTransform() const;

	UFUNCTION(BlueprintCallable, Category = "Cell|Box")
	ABoxActor* SpawnBoxAt(TSubclassOf<ABoxActor> BoxClass, const FIntPoint& TargetCoord);

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Preview")
	void BuildEditorPreview(FCellEditorPreviewContext& PreviewContext);
	virtual void BuildEditorPreview_Implementation(FCellEditorPreviewContext& PreviewContext);

	UFUNCTION(BlueprintCallable, Category = "Cell|Preview")
	ABoxActor* SpawnPreviewBoxAt(FCellEditorPreviewContext& PreviewContext, TSubclassOf<ABoxActor> BoxClass, const FIntPoint& TargetCoord);

	UFUNCTION(BlueprintCallable, Category = "Cell|Preview")
	APawn* SpawnPreviewPawn(FCellEditorPreviewContext& PreviewContext, TSubclassOf<APawn> PawnClass);

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	bool CanPlayerEnter(const FCellMoveContext& Context) const;
	virtual bool CanPlayerEnter_Implementation(const FCellMoveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	bool CanPlayerExit(const FCellMoveContext& Context) const;
	virtual bool CanPlayerExit_Implementation(const FCellMoveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	void OnPlayerEnter(const FCellMoveContext& Context);
	virtual void OnPlayerEnter_Implementation(const FCellMoveContext& Context);

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	void OnPlayerExit(const FCellMoveContext& Context);
	virtual void OnPlayerExit_Implementation(const FCellMoveContext& Context);

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	bool CanBoxEnter(const FCellMoveContext& Context) const;
	virtual bool CanBoxEnter_Implementation(const FCellMoveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	bool CanBoxExit(const FCellMoveContext& Context) const;
	virtual bool CanBoxExit_Implementation(const FCellMoveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	void OnBoxEnter(const FCellMoveContext& Context);
	virtual void OnBoxEnter_Implementation(const FCellMoveContext& Context);

	UFUNCTION(BlueprintNativeEvent, Category = "Cell|Events")
	void OnBoxExit(const FCellMoveContext& Context);
	virtual void OnBoxExit_Implementation(const FCellMoveContext& Context);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "1"))
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "2"))
	UStaticMeshComponent* CellMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "3"))
	UNiagaraComponent* CellNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell", meta = (DisplayPriority = "4"))
	bool bBlocksMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell", meta = (DisplayPriority = "5"))
	FIntPoint GridCoord;

private:
	UPROPERTY(Transient)
	TObjectPtr<APushBoxLevelRuntime> CachedRuntime;
};
