#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/MagicTourCharacter.h"
#include "Components/Border.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Components/CapsuleComponent.h"
#include "Components/FlightComponent.h"
#include "Components/HealthComponent.h"
#include "Components/ProgressBar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/MagicTourGameMode.h"
#include "Framework/MagicTourPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/HealthTestObserver.h"
#include "UI/MagicTourHUDWidget.h"
#include "UObject/UnrealType.h"
#include "World/Checkpoint.h"

namespace
{
	/** Keep respawn and UI integration independent of the open editor level. */
	struct FRespawnWorld
	{
		UGameInstance* Instance;
		UWorld* World;

		FRespawnWorld()
		{
			Instance = NewObject<UGameInstance>(GEngine);
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			World->GetWorldSettings()->DefaultGameMode = AMagicTourGameMode::StaticClass();
			World->SetGameMode(FURL());
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();

			AActor* Floor = World->SpawnActor<AActor>();
			UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
			Floor->SetRootComponent(Box);
			Box->SetBoxExtent(FVector(5000.0f, 5000.0f, 20.0f));
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->RegisterComponent();
			Floor->SetActorLocation(FVector(0.0f, 0.0f, -20.0f));
		}

		~FRespawnWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			Instance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerRespawnTest, "MagicTour.Health.PlayerRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerRespawnTest::RunTest(const FString& Parameters)
{
	UClass* PlayerClass = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	UClass* ControllerClass = LoadClass<AMagicTourPlayerController>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourPlayerController.BP_MagicTourPlayerController_C"));
	if (!TestNotNull(TEXT("Player Blueprint loads"), PlayerClass)
		|| !TestNotNull(TEXT("Player controller Blueprint loads"), ControllerClass)) { return false; }

	FRespawnWorld Scene;
	const FTransform Spawn(FRotator(0.0f, 35.0f, 0.0f), FVector(-1000.0f, 0.0f, 96.0f));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMagicTourCharacter* Player = Scene.World->SpawnActor<AMagicTourCharacter>(PlayerClass, Spawn, Params);
	AMagicTourPlayerController* Controller = Scene.World->SpawnActor<AMagicTourPlayerController>(ControllerClass);
	AMagicTourGameMode* Mode = Scene.World->GetAuthGameMode<AMagicTourGameMode>();
	if (!TestNotNull(TEXT("Player spawns"), Player)
		|| !TestNotNull(TEXT("Player controller spawns"), Controller)
		|| !TestNotNull(TEXT("Progress GameMode exists"), Mode)) { return false; }
	Controller->Possess(Player);
	const FTransform Start = Player->GetActorTransform();
	UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
	if (!TestNotNull(TEXT("Player owns health"), Health)) { return false; }
	const ECollisionEnabled::Type Collision = Player->GetCapsuleComponent()->GetCollisionEnabled();
	UClass* AnimClass = Player->GetMesh()->GetAnimClass();
	TestFalse(TEXT("Living player is not marked dead"), Controller->IsDead());
	TestFalse(TEXT("Respawn rejects a living player"), Controller->Respawn());
	TestTrue(TEXT("Rejected respawn keeps the controlled pawn"), Controller->GetPawn() == Player);
	// Attach the native HUD to the nonlocal test controller without manufacturing a viewport session.
	UMagicTourHUDWidget* HUD = NewObject<UMagicTourHUDWidget>(Scene.World);
	if (!TestTrue(TEXT("Gameplay HUD initializes"), HUD->Initialize())) { return false; }
	HUD->SetProgress(Mode);
	FindFProperty<FObjectPropertyBase>(AMagicTourPlayerController::StaticClass(), TEXT("HUDWidget"))
		->SetObjectPropertyValue_InContainer(Controller, HUD);
	FScriptDelegate Request;
	Request.BindUFunction(Controller, TEXT("RespawnClicked"));
	HUD->OnRespawn.AddUnique(Request);
	UProgressBar* Bar = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("HealthBar")));
	if (!TestNotNull(TEXT("HUD exposes the player's health bar"), Bar)) { return false; }

	// Advance a real checkpoint and change enemy health before replacing only the player.
	ACheckpoint* Checkpoint = Scene.World->SpawnActor<ACheckpoint>(ACheckpoint::StaticClass(),
		FVector(0.0f, 0.0f, 96.0f), FRotator::ZeroRotator);
	AEnemyCharacter* Enemy = Scene.World->SpawnActor<AEnemyCharacter>(AEnemyCharacter::StaticClass(),
		FVector(1800.0f, 0.0f, 96.0f), FRotator::ZeroRotator, Params);
	AEnemyCharacter* Body = Scene.World->SpawnActor<AEnemyCharacter>(AEnemyCharacter::StaticClass(),
		FVector(2200.0f, 0.0f, 96.0f), FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Checkpoint spawns"), Checkpoint)
		|| !TestNotNull(TEXT("Living enemy spawns"), Enemy)
		|| !TestNotNull(TEXT("Second enemy spawns"), Body)) { return false; }
	Player->SetActorLocation(FVector(0.0f, 0.0f, 96.0f));
	Player->GetCapsuleComponent()->UpdateOverlaps();
	if (!TestTrue(TEXT("Player completes the checkpoint"), Checkpoint->IsReached())) { return false; }
	Enemy->FindComponentByClass<UHealthComponent>()->ApplyDamage(50.0f);
	Body->FindComponentByClass<UHealthComponent>()->ApplyDamage(100.0f);
	const float Remaining = Body->GetLifeSpan();

	for (int32 Cycle = 0; Cycle < 2; ++Cycle)
	{
		// The second cycle starts in flight away from the spawn to catch cached-location regressions.
		if (Cycle == 1)
		{
			Player->FindComponentByClass<UFlightComponent>()->ToggleFlight();
			Player->SetActorLocation(FVector(800.0f, 0.0f, 700.0f));
		}
		Health->ApplyDamage(1000.0f);
		TestTrue(TEXT("Lethal damage marks the controller dead"), Controller->IsDead());
		TestTrue(TEXT("Lethal damage displays the death dialog"), HUD->IsDeathShown());
		TestEqual(TEXT("Lethal damage empties the live health bar"), Bar->GetPercent(), 0.0f);
		TestTrue(TEXT("Dead player has disabled collision"),
			Player->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		AMagicTourCharacter* OldPawn = Player;
		if (Cycle == 0)
		{
			HUD->GetRespawnButton()->OnClicked.Broadcast();
			if (!TestTrue(TEXT("Death dialog action respawns the player"), Controller->GetPawn() != OldPawn)) { return false; }
		}
		else if (!TestTrue(TEXT("Dead player can respawn again"), Controller->Respawn())) { return false; }
		Player = Cast<AMagicTourCharacter>(Controller->GetPawn());
		if (!TestNotNull(TEXT("Respawn possesses a character"), Player)) { return false; }
		Health = Player->FindComponentByClass<UHealthComponent>();
		UFlightComponent* Flight = Player->FindComponentByClass<UFlightComponent>();
		if (!TestNotNull(TEXT("Respawn restores health component"), Health)
			|| !TestNotNull(TEXT("Respawn restores flight component"), Flight)) { return false; }
		TestTrue(TEXT("Respawn creates a fresh pawn"), Player != OldPawn);
		TestTrue(TEXT("Previous corpse is destroyed after possession"), OldPawn->IsActorBeingDestroyed());
		TestEqual(TEXT("Respawn keeps the player Blueprint class"), Player->GetClass(), PlayerClass);
		// Character movement maintains a small floor gap; respawning may adjust it by a fraction of a centimeter.
		TestTrue(TEXT("Respawn restores the original horizontal spawn position"),
			FVector::DistSquared2D(Player->GetActorLocation(), Start.GetLocation()) < 0.01f);
		TestTrue(TEXT("Respawn preserves spawn height within the movement floor tolerance"),
			FMath::Abs(Player->GetActorLocation().Z - Start.GetLocation().Z) < 1.0f);
		TestTrue(TEXT("Respawn restores the original body rotation"), Player->GetActorRotation().Equals(Start.Rotator(), 0.01f));
		TestTrue(TEXT("Respawn preserves original body scale"), Player->GetActorScale3D().Equals(Start.GetScale3D(), 0.01f));
		TestTrue(TEXT("Respawn restores the starting control rotation"), Controller->GetControlRotation().Equals(Spawn.Rotator(), 0.01f));
		TestEqual(TEXT("Respawn restores full health"), Health->GetHealth(), 100.0f);
		TestFalse(TEXT("Respawn clears controller death state"), Controller->IsDead());
		TestFalse(TEXT("Respawn hides the controller's death dialog"), HUD->IsDeathShown());
		TestEqual(TEXT("Possession reconnects the HUD to full health"), Bar->GetPercent(), 1.0f);
		TestTrue(TEXT("Respawn restores capsule collision"), Player->GetCapsuleComponent()->GetCollisionEnabled() == Collision);
		TestTrue(TEXT("Respawn restores locomotion animation mode"),
			Player->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint);
		TestEqual(TEXT("Respawn restores the original animation class"), Player->GetMesh()->GetAnimClass(), AnimClass);
		TestNotNull(TEXT("Respawn has a live animation instance"), Player->GetMesh()->GetAnimInstance());
		TestFalse(TEXT("Respawn starts outside flight"), Flight->IsFlying());
		TestFalse(TEXT("Respawn leaves idle flight ticking disabled"), Flight->IsComponentTickEnabled());
		TestTrue(TEXT("Respawn restores movement"), Player->GetCharacterMovement()->MovementMode != MOVE_None);
		Flight->ToggleFlight();
		TestTrue(TEXT("Respawn allows starting flight again"), Flight->IsFlying());
		TestTrue(TEXT("Starting flight reenables flight ticking"), Flight->IsComponentTickEnabled());
		Flight->ToggleFlight();
		TestFalse(TEXT("Repeated respawn while alive is ignored"), Controller->Respawn());
		TestTrue(TEXT("Ignored repeat keeps the new pawn"), Controller->GetPawn() == Player);
		TestEqual(TEXT("Respawn preserves the GameMode instance"), Scene.World->GetAuthGameMode<AMagicTourGameMode>(), Mode);
		TestEqual(TEXT("Respawn preserves reached checkpoint progress"), Mode->GetCheckpointIndex(), 1);
		TestTrue(TEXT("Respawn preserves the completed checkpoint actor"), IsValid(Checkpoint) && Checkpoint->IsReached());
		TestTrue(TEXT("Respawn preserves the living enemy actor"), IsValid(Enemy) && !Enemy->IsActorBeingDestroyed());
		TestEqual(TEXT("Respawn preserves enemy damage"), Enemy->FindComponentByClass<UHealthComponent>()->GetHealth(), 50.0f);
		TestTrue(TEXT("Respawn preserves enemy death state"), Body->GetState() == EEnemyState::Dead);
		TestEqual(TEXT("Respawn does not restart the enemy removal timer"), Body->GetLifeSpan(), Remaining);
	}
	int32 Enemies = 0;
	for (TActorIterator<AEnemyCharacter> It(Scene.World); It; ++It) { ++Enemies; }
	TestEqual(TEXT("Player respawns never recreate enemies"), Enemies, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeathPromptTest, "MagicTour.HUD.DeathPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDeathPromptTest::RunTest(const FString& Parameters)
{
	FRespawnWorld Scene;
	UMagicTourHUDWidget* HUD = NewObject<UMagicTourHUDWidget>(Scene.World);
	if (!TestNotNull(TEXT("Native gameplay HUD exists"), HUD)
		|| !TestTrue(TEXT("Native gameplay HUD initializes"), HUD->Initialize())) { return false; }
	UButton* Button = HUD->GetRespawnButton();
	UBorder* Overlay = Cast<UBorder>(HUD->GetWidgetFromName(TEXT("DeathOverlay")));
	UBorder* Crosshair = Cast<UBorder>(HUD->GetWidgetFromName(TEXT("CrosshairH")));
	UTextBlock* Title = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("DeathTitle")));
	UTextBlock* Label = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("RespawnText")));
	if (!TestNotNull(TEXT("Death action button exists"), Button)
		|| !TestNotNull(TEXT("Death overlay exists"), Overlay)
		|| !TestNotNull(TEXT("Crosshair exists"), Crosshair)
		|| !TestNotNull(TEXT("Death title exists"), Title)
		|| !TestNotNull(TEXT("Respawn label exists"), Label)) { return false; }
	TestEqual(TEXT("Death title uses the intended English prompt"), Title->GetText().ToString(), FString(TEXT("You Died")));
	TestEqual(TEXT("Action uses the intended English label"), Label->GetText().ToString(), FString(TEXT("Respawn")));
	TestTrue(TEXT("Respawn action accepts keyboard focus"), Button->GetIsFocusable());
	UHealthTestObserver* Probe = NewObject<UHealthTestObserver>();
	HUD->OnRespawn.AddDynamic(Probe, &UHealthTestObserver::Died);
	TestFalse(TEXT("Death dialog starts hidden"), HUD->IsDeathShown());
	TestTrue(TEXT("Hidden death overlay occupies no layout space"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
	Button->OnClicked.Broadcast();
	TestEqual(TEXT("Hidden dialog ignores a stale button activation"), Probe->Deaths, 0);
	HUD->ShowDeath(true);
	TestTrue(TEXT("Death dialog tracks visible state"), HUD->IsDeathShown());
	TestTrue(TEXT("Death overlay is visible and blocks clicks behind it"), Overlay->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("Death hides the aiming crosshair"), Crosshair->GetVisibility() == ESlateVisibility::Collapsed);
	Button->OnClicked.Broadcast();
	TestEqual(TEXT("Visible action requests one respawn"), Probe->Deaths, 1);
	HUD->ShowDeath(false);
	TestFalse(TEXT("Respawn hides the dialog state"), HUD->IsDeathShown());
	TestTrue(TEXT("Respawn collapses the death overlay"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("Respawn restores the noninteractive crosshair"), Crosshair->GetVisibility() == ESlateVisibility::HitTestInvisible);
	Button->OnClicked.Broadcast();
	TestEqual(TEXT("Hidden dialog cannot request another respawn"), Probe->Deaths, 1);
	HUD->OnRespawn.RemoveDynamic(Probe, &UHealthTestObserver::Died);
	return true;
}

#endif
