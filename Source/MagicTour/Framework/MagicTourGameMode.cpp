// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/MagicTourGameMode.h"
#include "Characters/EnemyCharacter.h"
#include "Components/HealthComponent.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "MagicTourProgress"

AMagicTourGameMode::AMagicTourGameMode()
{
}

void AMagicTourGameMode::BeginPlay()
{
	Super::BeginPlay();
	// Enroll the whole placed encounter before combat can remove any of its actors.
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It) { RegisterEnemy(*It); }
}

void AMagicTourGameMode::RegisterEnemy(AEnemyCharacter* Enemy)
{
	if (!isVictory && IsValid(Enemy)) { Enemies.Add(Enemy); }
}

void AMagicTourGameMode::EnemyDied(AEnemyCharacter* Enemy)
{
	const UHealthComponent* Health = IsValid(Enemy) ? Enemy->FindComponentByClass<UHealthComponent>() : nullptr;
	if (isVictory || !Health || !Health->IsDead() || !Enemies.Contains(Enemy) || DeadEnemies.Contains(Enemy))
	{
		return;
	}
	DeadEnemies.Add(Enemy);
	if (!Enemies.IsEmpty() && GetEnemyRemaining() == 0)
	{
		// Commit before notifying UI so repeated or reentrant damage cannot duplicate victory.
		isVictory = true;
		OnVictory.Broadcast();
	}
}

bool AMagicTourGameMode::Complete()
{
	if (!isVictory || CurrentStage == EMagicTourStage::Complete) { return false; }
	CurrentStage = EMagicTourStage::Complete;
	OnCheckpointChanged.Broadcast(CheckpointIndex, CheckpointTotal, CurrentStage);
	return true;
}

bool AMagicTourGameMode::ReachCheckpoint(int32 Index, EMagicTourStage Stage)
{
	if (isVictory || Index != CheckpointIndex + 1 || Index > CheckpointTotal)
	{
		return false;
	}

	CheckpointIndex = Index;
	CurrentStage = Stage;
	OnCheckpointChanged.Broadcast(CheckpointIndex, CheckpointTotal, CurrentStage);
	return true;
}

FText AMagicTourGameMode::GetStageText() const
{
	switch (CurrentStage)
	{
	case EMagicTourStage::Complete:
		return LOCTEXT("Complete", "All Tasks Complete");
	case EMagicTourStage::ExploreForest:
		return LOCTEXT("ExploreForest", "Explore the Forest");
	case EMagicTourStage::ExploreRuins:
		return LOCTEXT("ExploreRuins", "Explore the Ruins");
	case EMagicTourStage::DefeatEnemies:
		return LOCTEXT("DefeatEnemies", "Defeat the Enemies");
	case EMagicTourStage::ReachForest:
	default:
		return LOCTEXT("ReachForest", "Reach the Forest Entrance");
	}
}

#undef LOCTEXT_NAMESPACE
