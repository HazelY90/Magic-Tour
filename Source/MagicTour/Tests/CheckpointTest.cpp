// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/MagicTourGameMode.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCheckpointProgressTest,
	"MagicTour.Progress.CheckpointOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCheckpointProgressTest::RunTest(const FString& Parameters)
{
	AMagicTourGameMode* GameMode = NewObject<AMagicTourGameMode>();
	if (!TestNotNull(TEXT("GameMode can be created"), GameMode))
	{
		return false;
	}

	TestEqual(TEXT("Route starts before checkpoint one"), GameMode->GetCheckpointIndex(), 0);
	TestEqual(TEXT("Initial task points to the forest entrance"), GameMode->GetStage(), EMagicTourStage::ReachForest);
	TestFalse(TEXT("Checkpoint two cannot be reached first"), GameMode->ReachCheckpoint(2, EMagicTourStage::ExploreRuins));
	TestTrue(TEXT("Checkpoint one advances the route"), GameMode->ReachCheckpoint(1, EMagicTourStage::ExploreForest));
	TestEqual(TEXT("Checkpoint one starts forest exploration"), GameMode->GetStage(), EMagicTourStage::ExploreForest);
	TestEqual(TEXT("Checkpoint one exposes the forest task text"), GameMode->GetStageText().ToString(), FString(TEXT("Explore the Forest")));
	TestFalse(TEXT("Checkpoint one cannot be counted twice"), GameMode->ReachCheckpoint(1, EMagicTourStage::ExploreForest));
	TestTrue(TEXT("Checkpoint two advances the route"), GameMode->ReachCheckpoint(2, EMagicTourStage::ExploreRuins));
	TestEqual(TEXT("Checkpoint two starts ruins exploration"), GameMode->GetStage(), EMagicTourStage::ExploreRuins);
	TestEqual(TEXT("Checkpoint two exposes the ruins task text"), GameMode->GetStageText().ToString(), FString(TEXT("Explore the Ruins")));
	TestTrue(TEXT("Checkpoint three advances the route"), GameMode->ReachCheckpoint(3, EMagicTourStage::DefeatEnemies));
	TestEqual(TEXT("Checkpoint three starts the combat task"), GameMode->GetStage(), EMagicTourStage::DefeatEnemies);
	TestEqual(TEXT("Checkpoint three exposes the combat task text"), GameMode->GetStageText().ToString(), FString(TEXT("Defeat the Enemies")));
	TestFalse(TEXT("Route cannot advance past the configured total"), GameMode->ReachCheckpoint(4, EMagicTourStage::DefeatEnemies));

	return true;
}

#endif
