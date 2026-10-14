#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EnemyCharacter.h"
#include "Characters/MagicTourCharacter.h"
#include "Components/Border.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Components/HealthComponent.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/MagicTourGameMode.h"
#include "Framework/MagicTourPlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/HealthTestObserver.h"
#include "UI/MagicTourHUDWidget.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Exercise encounter completion independently of the current editor map. */
	struct FVictoryWorld
	{
		UGameInstance* Instance;
		UWorld* World;
		TArray<AEnemyCharacter*> Enemies;

		explicit FVictoryWorld(bool isPlaced = false)
		{
			Instance = NewObject<UGameInstance>(GEngine);
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			World->GetWorldSettings()->DefaultGameMode = AMagicTourGameMode::StaticClass();
			World->SetGameMode(FURL());
			World->InitializeActorsForPlay(FURL());

			AActor* Floor = World->SpawnActor<AActor>();
			UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
			Floor->SetRootComponent(Box);
			Box->SetBoxExtent(FVector(5000.0f, 5000.0f, 20.0f));
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->RegisterComponent();
			Floor->SetActorLocation(FVector(0.0f, 0.0f, -20.0f));

			// Preexisting actors exercise both GameMode discovery and native BeginPlay registration.
			if (isPlaced) { SpawnEnemies(); }
			World->BeginPlay();
		}

		~FVictoryWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			Instance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		void SpawnEnemies()
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Enemies.Add(World->SpawnActor<AEnemyCharacter>(AEnemyCharacter::StaticClass(),
					FVector(1600.0f + 400.0f * Index, 0.0f, 96.0f), FRotator::ZeroRotator, Params));
			}
		}

		bool IsReady(FAutomationTestBase& Test) const
		{
			if (!Test.TestNotNull(TEXT("Encounter GameMode exists"), Mode())) { return false; }
			for (AEnemyCharacter* Enemy : Enemies)
			{
				if (!Test.TestNotNull(TEXT("Encounter enemy spawns"), Enemy)
					|| !Test.TestNotNull(TEXT("Encounter enemy owns health"), Enemy->FindComponentByClass<UHealthComponent>()))
				{
					return false;
				}
			}
			return true;
		}

		AMagicTourGameMode* Mode() const { return World->GetAuthGameMode<AMagicTourGameMode>(); }

		void Defeat(int32 Index) const
		{
			Enemies[Index]->FindComponentByClass<UHealthComponent>()->ApplyDamage(1000.0f);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVictoryProgressTest, "MagicTour.Progress.EnemyVictory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVictoryProgressTest::RunTest(const FString& Parameters)
{
	// An empty map must not manufacture a victory; only enrolled opponents count.
	{
		FVictoryWorld Empty;
		if (!Empty.IsReady(*this)) { return false; }
		TestEqual(TEXT("An empty encounter starts with no enrolled enemies"), Empty.Mode()->GetEnemyTotal(), 0);
		TestFalse(TEXT("An empty encounter does not count as victory"), Empty.Mode()->IsVictory());
		TestFalse(TEXT("Task completion rejects an empty encounter"), Empty.Mode()->Complete());
		Empty.SpawnEnemies();
		if (!Empty.IsReady(*this)) { return false; }
		TestEqual(TEXT("Enemies spawned during play are enrolled"), Empty.Mode()->GetEnemyTotal(), 3);
		TestEqual(TEXT("All newly spawned enemies remain alive"), Empty.Mode()->GetEnemyRemaining(), 3);
	}

	FVictoryWorld Scene(true);
	if (!Scene.IsReady(*this)) { return false; }
	AMagicTourGameMode* Mode = Scene.Mode();
	UHealthTestObserver* Probe = NewObject<UHealthTestObserver>();
	Mode->OnVictory.AddDynamic(Probe, &UHealthTestObserver::Died);
	TestEqual(TEXT("Placed enemies are enrolled exactly once"), Mode->GetEnemyTotal(), 3);
	TestEqual(TEXT("Three placed enemies remain alive"), Mode->GetEnemyRemaining(), 3);
	TestFalse(TEXT("Living enemies keep victory unavailable"), Mode->IsVictory());
	TestFalse(TEXT("Confirmation cannot complete a living encounter"), Mode->Complete());
	Mode->RegisterEnemy(nullptr);
	Mode->EnemyDied(nullptr);
	for (AEnemyCharacter* Enemy : Scene.Enemies)
	{
		Mode->RegisterEnemy(Enemy);
		Mode->EnemyDied(Enemy);
	}
	TestEqual(TEXT("Duplicate registration does not enlarge the encounter"), Mode->GetEnemyTotal(), 3);
	TestEqual(TEXT("Reports from living enemies do not reduce the count"), Mode->GetEnemyRemaining(), 3);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		Scene.Defeat(Index);
		AEnemyCharacter* Enemy = Scene.Enemies[Index];
		TestTrue(TEXT("Lethal damage enters the existing enemy death state"), Enemy->GetState() == EEnemyState::Dead);
		TestEqual(TEXT("Each actual death reduces the live enemy count once"), Mode->GetEnemyRemaining(), 2 - Index);
		TestEqual(TEXT("Each death retains the existing five-second removal delay"), Enemy->GetLifeSpan(), 5.0f);
		TestFalse(TEXT("A newly defeated enemy remains present until its removal delay"), Enemy->IsActorBeingDestroyed());
		TestEqual(TEXT("Victory requires the final real enemy death"), Mode->IsVictory(), Index == 2);
		TestEqual(TEXT("The victory event is emitted once at the final death"), Probe->Deaths, Index == 2 ? 1 : 0);
		// Reentrant or stale notifications must not count the same corpse twice.
		Mode->EnemyDied(Enemy);
		Mode->RegisterEnemy(Enemy);
		Scene.Defeat(Index);
		TestEqual(TEXT("Repeated reports cannot reduce the count again"), Mode->GetEnemyRemaining(), 2 - Index);
		TestEqual(TEXT("Repeated reports cannot repeat victory"), Probe->Deaths, Index == 2 ? 1 : 0);
	}
	TestEqual(TEXT("Victory retains the original task until acknowledged"), Mode->GetStage(), EMagicTourStage::ReachForest);
	TestTrue(TEXT("A defeated encounter can be acknowledged"), Mode->Complete());
	TestEqual(TEXT("Acknowledgement advances to the completed stage"), Mode->GetStage(), EMagicTourStage::Complete);
	TestEqual(TEXT("The completed stage exposes the requested task label"),
		Mode->GetStageText().ToString(), FString(TEXT("All Tasks Complete")));
	TestFalse(TEXT("Completion acknowledgement is consumed once"), Mode->Complete());
	TestFalse(TEXT("A later valid checkpoint cannot regress a completed task"),
		Mode->ReachCheckpoint(1, EMagicTourStage::ExploreForest));
	TestEqual(TEXT("Rejected checkpoint leaves the completed stage intact"), Mode->GetStage(), EMagicTourStage::Complete);
	TestEqual(TEXT("Victory counting does not manufacture checkpoint progress"), Mode->GetCheckpointIndex(), 0);
	Mode->OnVictory.RemoveDynamic(Probe, &UHealthTestObserver::Died);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVictoryPromptTest, "MagicTour.HUD.VictoryPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVictoryPromptTest::RunTest(const FString& Parameters)
{
	FVictoryWorld Scene;
	UMagicTourHUDWidget* HUD = NewObject<UMagicTourHUDWidget>(Scene.World);
	if (!TestTrue(TEXT("Gameplay HUD initializes"), HUD->Initialize())) { return false; }
	UButton* Button = HUD->GetConfirmButton();
	UBorder* Overlay = Cast<UBorder>(HUD->GetWidgetFromName(TEXT("VictoryOverlay")));
	UBorder* Crosshair = Cast<UBorder>(HUD->GetWidgetFromName(TEXT("CrosshairH")));
	UTextBlock* Title = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("VictoryTitle")));
	UTextBlock* Label = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("ConfirmText")));
	if (!TestNotNull(TEXT("Victory action button exists"), Button)
		|| !TestNotNull(TEXT("Victory overlay exists"), Overlay)
		|| !TestNotNull(TEXT("Aiming crosshair exists"), Crosshair)
		|| !TestNotNull(TEXT("Victory title exists"), Title)
		|| !TestNotNull(TEXT("Confirmation label exists"), Label)) { return false; }
	TestEqual(TEXT("Victory uses the intended English title"), Title->GetText().ToString(), FString(TEXT("Victory")));
	TestEqual(TEXT("Confirmation uses the intended English action"), Label->GetText().ToString(), FString(TEXT("OK")));
	TestTrue(TEXT("Victory action accepts keyboard focus"), Button->GetIsFocusable());
	UHealthTestObserver* Probe = NewObject<UHealthTestObserver>();
	HUD->OnConfirm.AddDynamic(Probe, &UHealthTestObserver::Died);
	TestFalse(TEXT("Victory dialog starts hidden"), HUD->IsVictoryShown());
	TestTrue(TEXT("Hidden victory overlay occupies no layout space"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
	Button->OnClicked.Broadcast();
	TestEqual(TEXT("Hidden victory action ignores stale activation"), Probe->Deaths, 0);
	HUD->ShowVictory(true);
	TestTrue(TEXT("Victory dialog tracks its pending state"), HUD->IsVictoryShown());
	TestTrue(TEXT("Victory scrim is visible and blocks gameplay clicks"), Overlay->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("Victory hides the aiming crosshair"), Crosshair->GetVisibility() == ESlateVisibility::Collapsed);
	HUD->ShowDeath(true);
	TestTrue(TEXT("Death keeps victory pending"), HUD->IsVictoryShown());
	TestTrue(TEXT("Death hides the victory overlay"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
	Button->OnClicked.Broadcast();
	TestEqual(TEXT("Death prevents victory acknowledgement"), Probe->Deaths, 0);
	HUD->ShowDeath(false);
	TestTrue(TEXT("Leaving death restores the pending victory overlay"), Overlay->GetVisibility() == ESlateVisibility::Visible);
	Button->OnClicked.Broadcast();
	TestEqual(TEXT("A visible victory action requests one acknowledgement"), Probe->Deaths, 1);
	HUD->ShowVictory(false);
	TestFalse(TEXT("Acknowledgement clears the visible victory state"), HUD->IsVictoryShown());
	TestTrue(TEXT("Acknowledgement collapses the victory overlay"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("Acknowledgement restores the noninteractive crosshair"),
		Crosshair->GetVisibility() == ESlateVisibility::HitTestInvisible);
	Button->OnClicked.Broadcast();
	TestEqual(TEXT("Hidden action cannot request another acknowledgement"), Probe->Deaths, 1);
	HUD->OnConfirm.RemoveDynamic(Probe, &UHealthTestObserver::Died);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVictoryControllerTest, "MagicTour.Progress.VictoryRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVictoryControllerTest::RunTest(const FString& Parameters)
{
	UClass* PlayerClass = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	UClass* ControllerClass = LoadClass<AMagicTourPlayerController>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourPlayerController.BP_MagicTourPlayerController_C"));
	if (!TestNotNull(TEXT("Player Blueprint loads"), PlayerClass)
		|| !TestNotNull(TEXT("Player controller Blueprint loads"), ControllerClass)) { return false; }

	// Both event orders can happen when a projectile survives the player's lethal hit.
	for (int32 Order = 0; Order < 2; ++Order)
	{
		FVictoryWorld Scene(true);
		if (!Scene.IsReady(*this)) { return false; }
		AMagicTourGameMode* Mode = Scene.Mode();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMagicTourCharacter* Player = Scene.World->SpawnActor<AMagicTourCharacter>(PlayerClass,
			FVector(-1000.0f, 0.0f, 96.0f), FRotator::ZeroRotator, Params);
		AMagicTourPlayerController* Controller = Scene.World->SpawnActor<AMagicTourPlayerController>(ControllerClass);
		if (!TestNotNull(TEXT("Player spawns"), Player)
			|| !TestNotNull(TEXT("Player controller spawns"), Controller)) { return false; }
		Controller->Possess(Player);
		UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
		if (!TestNotNull(TEXT("Player owns health"), Health)) { return false; }
		// Attach the real native HUD without manufacturing a local viewport session.
		UMagicTourHUDWidget* HUD = NewObject<UMagicTourHUDWidget>(Scene.World);
		if (!TestTrue(TEXT("Controller gameplay HUD initializes"), HUD->Initialize())) { return false; }
		HUD->SetProgress(Mode);
		FObjectPropertyBase* HUDProp = FindFProperty<FObjectPropertyBase>(
			AMagicTourPlayerController::StaticClass(), TEXT("HUDWidget"));
		if (!TestNotNull(TEXT("Controller exposes the persistent HUD property"), HUDProp)) { return false; }
		HUDProp->SetObjectPropertyValue_InContainer(Controller, HUD);
		FScriptDelegate Confirm;
		Confirm.BindUFunction(Controller, TEXT("VictoryClicked"));
		HUD->OnConfirm.AddUnique(Confirm);
		FScriptDelegate Respawn;
		Respawn.BindUFunction(Controller, TEXT("RespawnClicked"));
		HUD->OnRespawn.AddUnique(Respawn);
		UBorder* Overlay = Cast<UBorder>(HUD->GetWidgetFromName(TEXT("VictoryOverlay")));
		UTextBlock* Task = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("TaskText")));
		if (!TestNotNull(TEXT("Controller victory overlay exists"), Overlay)
			|| !TestNotNull(TEXT("Live task label exists"), Task)
			|| !TestNotNull(TEXT("Controller confirmation button exists"), HUD->GetConfirmButton())
			|| !TestNotNull(TEXT("Controller respawn button exists"), HUD->GetRespawnButton())) { return false; }
		TestFalse(TEXT("Controller cannot confirm before victory"), Controller->ConfirmVictory());
		TestFalse(TEXT("Controller starts without pending victory"), Controller->IsVictoryPending());
		Mode->ReachCheckpoint(1, EMagicTourStage::ExploreForest);
		Mode->ReachCheckpoint(2, EMagicTourStage::ExploreRuins);
		Mode->ReachCheckpoint(3, EMagicTourStage::DefeatEnemies);
		Scene.Defeat(0);
		Scene.Defeat(1);
		TestFalse(TEXT("Surviving enemy leaves confirmation unavailable"), Controller->ConfirmVictory());
		if (Order == 0) { Health->ApplyDamage(1000.0f); }
		Scene.Defeat(2);
		TestTrue(TEXT("The final enemy immediately notifies the controller"), Controller->IsVictoryPending());
		TestEqual(TEXT("The final enemy leaves the combat task pending confirmation"),
			Mode->GetStage(), EMagicTourStage::DefeatEnemies);
		TestEqual(TEXT("The live HUD retains combat guidance before acknowledgement"),
			Task->GetText().ToString(), FString(TEXT("Task: Defeat the Enemies")));
		if (Order == 1)
		{
			TestTrue(TEXT("Living player immediately sees victory"), HUD->IsVictoryShown());
			TestTrue(TEXT("Living player sees the victory scrim"), Overlay->GetVisibility() == ESlateVisibility::Visible);
			Health->ApplyDamage(1000.0f);
		}
		TestTrue(TEXT("Both event orders retain player death"), Controller->IsDead());
		TestTrue(TEXT("Death dialog has priority over victory"), HUD->IsDeathShown());
		TestTrue(TEXT("Victory scrim stays hidden while dead"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
		TestFalse(TEXT("Dead player cannot acknowledge victory directly"), Controller->ConfirmVictory());
		HUD->GetConfirmButton()->OnClicked.Broadcast();
		TestTrue(TEXT("A stale victory click keeps acknowledgement pending"), Controller->IsVictoryPending());
		TestEqual(TEXT("A stale victory click cannot advance the task"), Mode->GetStage(), EMagicTourStage::DefeatEnemies);
		HUD->GetRespawnButton()->OnClicked.Broadcast();
		AMagicTourCharacter* Pawn = Cast<AMagicTourCharacter>(Controller->GetPawn());
		if (!TestNotNull(TEXT("Death action restores a player pawn"), Pawn)
			|| !TestTrue(TEXT("Death action creates a fresh player"), Pawn != Player)) { return false; }
		TestFalse(TEXT("Respawn clears death before presenting victory"), Controller->IsDead());
		TestFalse(TEXT("Respawn closes the death dialog"), HUD->IsDeathShown());
		TestTrue(TEXT("Respawn preserves the pending victory"), Controller->IsVictoryPending());
		TestTrue(TEXT("Respawn reveals the victory dialog"), HUD->IsVictoryShown());
		TestTrue(TEXT("Respawn reveals the victory scrim"), Overlay->GetVisibility() == ESlateVisibility::Visible);
		TestEqual(TEXT("Respawn preserves the defeated encounter"), Mode->GetEnemyRemaining(), 0);
		TestEqual(TEXT("Respawn restores full player health"), Pawn->FindComponentByClass<UHealthComponent>()->GetHealth(), 100.0f);
		HUD->GetConfirmButton()->OnClicked.Broadcast();
		TestFalse(TEXT("Confirmation clears the controller's pending victory"), Controller->IsVictoryPending());
		TestFalse(TEXT("Confirmation dismisses the victory dialog"), HUD->IsVictoryShown());
		TestTrue(TEXT("Confirmation collapses the scrim"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
		TestEqual(TEXT("Confirmation advances the GameMode to complete"), Mode->GetStage(), EMagicTourStage::Complete);
		TestEqual(TEXT("Confirmation immediately updates the live task display"),
			Task->GetText().ToString(), FString(TEXT("Task: All Tasks Complete")));
		TestFalse(TEXT("Repeated controller acknowledgement is ignored"), Controller->ConfirmVictory());
		HUD->GetConfirmButton()->OnClicked.Broadcast();
		TestEqual(TEXT("A stale confirmation leaves the completed task intact"), Mode->GetStage(), EMagicTourStage::Complete);
	}
	return true;
}

#endif
