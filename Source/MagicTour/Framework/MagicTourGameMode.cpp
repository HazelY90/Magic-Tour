// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/MagicTourGameMode.h"

#define LOCTEXT_NAMESPACE "MagicTourProgress"

AMagicTourGameMode::AMagicTourGameMode()
{
}

bool AMagicTourGameMode::ReachCheckpoint(int32 Index, EMagicTourStage Stage)
{
	if (Index != CheckpointIndex + 1 || Index > CheckpointTotal)
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
