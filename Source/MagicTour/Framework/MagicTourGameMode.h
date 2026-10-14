// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MagicTourGameMode.generated.h"

class AEnemyCharacter;

UENUM(BlueprintType)
enum class EMagicTourStage : uint8
{
	ReachForest,
	ExploreForest,
	ExploreRuins,
	DefeatEnemies,
	Complete
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCheckpointChanged, int32, ReachedIndex, int32, TotalCheckpoints, EMagicTourStage, Stage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVictory);

/**
 *  GameMode for the third-person gameplay prototype.
 */
UCLASS()
class AMagicTourGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMagicTourGameMode();

	/** Track the encounter independently of corpse lifetime and route progress. */
	void RegisterEnemy(AEnemyCharacter* Enemy);
	void EnemyDied(AEnemyCharacter* Enemy);

	bool IsVictory() const { return isVictory; }
	int32 GetEnemyTotal() const { return Enemies.Num(); }
	int32 GetEnemyRemaining() const { return Enemies.Num() - DeadEnemies.Num(); }

	/** Acknowledge victory once and show the completed task stage. */
	bool Complete();

	UPROPERTY(BlueprintAssignable, Category="Progress")
	FOnVictory OnVictory;

	/** Accepts only the next checkpoint in the configured route. */
	UFUNCTION(BlueprintCallable, Category="Progress")
	bool ReachCheckpoint(int32 Index, EMagicTourStage Stage);

	/** Returns the last checkpoint reached by the player. */
	UFUNCTION(BlueprintPure, Category="Progress")
	int32 GetCheckpointIndex() const { return CheckpointIndex; }

	/** Returns the number of checkpoints in the route. */
	UFUNCTION(BlueprintPure, Category="Progress")
	int32 GetCheckpointTotal() const { return CheckpointTotal; }

	/** Returns the current task stage. */
	UFUNCTION(BlueprintPure, Category="Progress")
	EMagicTourStage GetStage() const { return CurrentStage; }

	/** Returns localized text for the current task stage. */
	UFUNCTION(BlueprintPure, Category="Progress")
	FText GetStageText() const;

	/** Broadcast when route progress or victory confirmation changes the task. */
	UPROPERTY(BlueprintAssignable, Category="Progress")
	FOnCheckpointChanged OnCheckpointChanged;

protected:
	virtual void BeginPlay() override;

	TSet<TWeakObjectPtr<AEnemyCharacter>> Enemies;
	TSet<TWeakObjectPtr<AEnemyCharacter>> DeadEnemies;
	bool isVictory = false;
	/** Number of ordered checkpoints required before combat. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Progress", meta=(ClampMin="1"))
	int32 CheckpointTotal = 3;

	/** Last checkpoint reached; zero means the route has not started. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Progress")
	int32 CheckpointIndex = 0;

	/** Current objective exposed to Blueprint presentation. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Progress")
	EMagicTourStage CurrentStage = EMagicTourStage::ReachForest;
};

