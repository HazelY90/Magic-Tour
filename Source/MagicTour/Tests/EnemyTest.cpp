#if WITH_DEV_AUTOMATION_TESTS

#include "AI/EnemyAIController.h"
#include "Characters/EnemyCharacter.h"
#include "Combat/SpellProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/MagicTourGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Tests/HealthTestObserver.h"
#include "Tests/SpellTestController.h"
#include "UObject/UnrealType.h"
#include "World/CombatArena.h"
#include "World/Checkpoint.h"

namespace
{
	/** Exercise real possession, animation, and damage without changing the editor map. */
	struct FEnemyWorld
	{
		UGameInstance* Instance;
		UWorld* World;
		ACombatArena* Arena;
		AMagicTourCharacter* Player;
		APlayerController* Controller;
		AEnemyCharacter* Enemy;

		FEnemyWorld()
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
			Box->SetBoxExtent(FVector(3000.0f, 3000.0f, 20.0f));
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->RegisterComponent();
			Floor->SetActorLocation(FVector(0.0f, 0.0f, -20.0f));

			Arena = World->SpawnActor<ACombatArena>(ACombatArena::StaticClass(),
				FVector(0.0f, 0.0f, 500.0f), FRotator::ZeroRotator);
			UClass* Class = LoadClass<AMagicTourCharacter>(nullptr,
				TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Player = Class ? World->SpawnActor<AMagicTourCharacter>(Class,
				FVector(600.0f, 0.0f, 96.0f), FRotator::ZeroRotator, Params) : nullptr;
			Controller = World->SpawnActor<APlayerController>();
			if (Controller && Player) { Controller->Possess(Player); }
			Enemy = SpawnEnemy(FVector(0.0f, 0.0f, 96.0f));
		}

		~FEnemyWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			Instance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		AEnemyCharacter* SpawnEnemy(const FVector& Pos)
		{
			const FTransform Transform(FRotator::ZeroRotator, Pos);
			AEnemyCharacter* Actor = World->SpawnActorDeferred<AEnemyCharacter>(
				AEnemyCharacter::StaticClass(), Transform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Actor)
			{
				Actor->Arena = Arena;
				Actor->FinishSpawning(Transform);
			}
			return Actor;
		}

		bool IsReady(FAutomationTestBase& Test) const
		{
			return Test.TestNotNull(TEXT("Arena spawns"), Arena)
				&& Test.TestNotNull(TEXT("Player Blueprint spawns"), Player)
				&& Test.TestNotNull(TEXT("Player controller spawns"), Controller)
				&& Test.TestNotNull(TEXT("Native enemy spawns"), Enemy)
				&& Test.TestNotNull(TEXT("Native enemy owns AI controller"),
					Cast<AEnemyAIController>(Enemy->GetController()));
		}

		void Stand() const
		{
			Enemy->GetCharacterMovement()->StopMovementImmediately();
			Enemy->ConsumeMovementInputVector();
			Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			Enemy->GetController()->SetControlRotation(FRotator::ZeroRotator);
			FindFProperty<FFloatProperty>(AMagicTourCharacter::StaticClass(), TEXT("Cooldown"))
				->SetPropertyValue_InContainer(Enemy, 0.0f);
		}

		void Advance(float Delta) const
		{
			USkeletalMeshComponent* Mesh = Enemy->GetMesh();
			Mesh->TickAnimation(Delta, false);
			Mesh->RefreshBoneTransforms();
			Mesh->AllowQueuedAnimEventsNextDispatch();
			Mesh->ConditionallyDispatchQueuedAnimEvents();
		}

		ASpellProjectile* Spell() const
		{
			for (TActorIterator<ASpellProjectile> It(World); It; ++It)
			{
				if (!It->IsActorBeingDestroyed()) { return *It; }
			}
			return nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetTest, "MagicTour.Enemy.TargetAndBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyTargetTest::RunTest(const FString& Parameters)
{
	FEnemyWorld Scene;
	if (!Scene.IsReady(*this)) { return false; }
	ACombatArena* Arena = Scene.Arena;
	TestTrue(TEXT("Arena includes the player position"), Arena->IsInside(Scene.Player->GetActorLocation()));
	TestFalse(TEXT("Arena rejects positions beyond its radius"),
		Arena->IsInside(Arena->GetActorLocation() + FVector(Arena->Radius + 100.0f, 0.0f, 0.0f)));
	TestFalse(TEXT("Arena rejects positions beyond its height"),
		Arena->IsInside(Arena->GetActorLocation() + FVector(0.0f, 0.0f, Arena->HalfHeight + 100.0f)));
	const TArray<FVector> Inside = {Scene.Enemy->GetActorLocation(), Scene.Player->GetActorLocation()};
	TestTrue(TEXT("Path wholly inside the arena is accepted"), Arena->IsPathInside(Inside));
	TArray<FVector> Outside = Inside;
	Outside.Insert(Arena->GetActorLocation() + FVector(Arena->Radius + 100.0f, 0.0f, 0.0f), 1);
	TestFalse(TEXT("A path that leaves and reenters the arena is rejected"), Arena->IsPathInside(Outside));

	Scene.Enemy->SetTarget(Scene.Player);
	TestTrue(TEXT("Living controlled player inside the arena is eligible"), Scene.Enemy->IsTargetValid());
	Scene.Player->SetActorLocation(FVector(600.0f, 250.0f, 96.0f));
	FVector Pos, Dir;
	TestTrue(TEXT("Enemy aim resolves toward the target"), Scene.Enemy->GetAim(Pos, Dir));
	TestTrue(TEXT("Enemy aim uses the player position instead of a camera ray"),
		Dir.Equals((Scene.Player->GetActorLocation() - Pos).GetSafeNormal(), 0.001f));
	Scene.Player->SetActorLocation(FVector(600.0f, -250.0f, 96.0f));
	TestTrue(TEXT("Aim can be refreshed after target movement"), Scene.Enemy->GetAim(Pos, Dir));
	TestTrue(TEXT("Refreshed aim follows the current target position"),
		Dir.Equals((Scene.Player->GetActorLocation() - Pos).GetSafeNormal(), 0.001f));
	Scene.Player->SetActorLocation(FVector(3000.0f, 0.0f, 96.0f));
	TestFalse(TEXT("Player outside the arena is ineligible"), Scene.Enemy->IsTargetValid());
	TestFalse(TEXT("Enemy cannot aim at a player outside the arena"), Scene.Enemy->GetAim(Pos, Dir));
	Scene.Player->SetActorLocation(FVector(600.0f, 0.0f, 96.0f));
	Scene.Controller->UnPossess();
	TestFalse(TEXT("Uncontrolled character is not a player target"), Scene.Enemy->IsTargetValid());
	Scene.Controller->Possess(Scene.Player);
	Scene.Player->FindComponentByClass<UHealthComponent>()->ApplyDamage(1000.0f);
	TestFalse(TEXT("Dead player is ineligible"), Scene.Enemy->IsTargetValid());
	Scene.Enemy->SetTarget(nullptr);
	TestFalse(TEXT("Missing target is ineligible"), Scene.Enemy->IsTargetValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyActivationTest, "MagicTour.Enemy.Activation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyActivationTest::RunTest(const FString& Parameters)
{
	FEnemyWorld Scene;
	if (!Scene.IsReady(*this)) { return false; }
	AEnemyAIController* AI = Cast<AEnemyAIController>(Scene.Enemy->GetController());
	TestEqual(TEXT("AI lookup identifies the controlled player"),
		UGameplayStatics::GetPlayerPawn(Scene.World, 0), static_cast<APawn*>(Scene.Player));
	Scene.Stand();
	Scene.Player->SetActorLocation(FVector(3000.0f, 0.0f, 96.0f));
	AI->Tick(0.15f);
	TestTrue(TEXT("Enemy patrols while the player is outside the ruins"),
		Scene.Enemy->GetState() == EEnemyState::Patrol);
	TestNull(TEXT("Outside player is not retained as a target"), Scene.Enemy->GetTarget());
	TestFalse(TEXT("Patrol does not start a spell against the outside player"), Scene.Enemy->IsCasting());

	Scene.Player->SetActorLocation(FVector(600.0f, 0.0f, 96.0f));
	AI->Tick(0.15f);
	TestEqual(TEXT("Entering the ruins acquires the player"), Scene.Enemy->GetTarget(), Scene.Player);
	TestTrue(TEXT("Visible player within range starts attacking"),
		Scene.Enemy->GetState() == EEnemyState::Attack);
	TestTrue(TEXT("AI starts the shared stationary casting action"), Scene.Enemy->IsCasting());
	TestTrue(TEXT("AI stops movement before casting"),
		Scene.Enemy->GetCharacterMovement()->Velocity.IsNearlyZero());

	Scene.Player->SetActorLocation(FVector(3000.0f, 0.0f, 96.0f));
	AI->Tick(0.15f);
	TestNull(TEXT("Leaving the ruins releases the player target"), Scene.Enemy->GetTarget());
	TestFalse(TEXT("Leaving before release cancels the pending attack"), Scene.Enemy->IsCasting());
	TestTrue(TEXT("Leaving the ruins returns the enemy to patrol"),
		Scene.Enemy->GetState() == EEnemyState::Patrol);
	Scene.Advance(0.4f);
	TestNull(TEXT("Canceled boundary attack cannot release a projectile"), Scene.Spell());

	Scene.Player->SetActorLocation(FVector(1700.0f, 0.0f, 96.0f));
	AI->Tick(0.15f);
	TestEqual(TEXT("Reentering the ruins reacquires the player"), Scene.Enemy->GetTarget(), Scene.Player);
	TestTrue(TEXT("Player beyond attack range starts pursuit"),
		Scene.Enemy->GetState() == EEnemyState::Chase);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyCheckpointTest, "MagicTour.Enemy.CheckpointIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyCheckpointTest::RunTest(const FString& Parameters)
{
	FEnemyWorld Scene;
	if (!Scene.IsReady(*this)) { return false; }
	AMagicTourGameMode* Mode = Scene.World->GetAuthGameMode<AMagicTourGameMode>();
	ACheckpoint* Checkpoint = Scene.World->SpawnActor<ACheckpoint>(ACheckpoint::StaticClass(),
		FVector(0.0f, 0.0f, 96.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Ordered checkpoint spawns"), Checkpoint)
		|| !TestNotNull(TEXT("Route GameMode is authoritative"), Mode)) { return false; }
	UBoxComponent* Trigger = Checkpoint->FindComponentByClass<UBoxComponent>();
	Trigger->UpdateOverlaps();
	TestTrue(TEXT("Checkpoint actually overlaps an AI character"), Trigger->IsOverlappingActor(Scene.Enemy));
	TestFalse(TEXT("Enemy overlap does not complete the checkpoint"), Checkpoint->IsReached());
	TestEqual(TEXT("Enemy overlap leaves route progress unchanged"), Mode->GetCheckpointIndex(), 0);
	Scene.Player->SetActorLocation(FVector(100.0f, 0.0f, 96.0f));
	Scene.Player->GetCapsuleComponent()->UpdateOverlaps();
	TestTrue(TEXT("Player overlap still completes the same checkpoint"), Checkpoint->IsReached());
	TestEqual(TEXT("Player overlap advances route progress once"), Mode->GetCheckpointIndex(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyCastTest, "MagicTour.Enemy.CastAndDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyCastTest::RunTest(const FString& Parameters)
{
	FEnemyWorld Scene;
	if (!Scene.IsReady(*this)) { return false; }
	UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr,
		TEXT("/Game/MagicTour/Characters/Animations/AM_Cast.AM_Cast"));
	if (!TestNotNull(TEXT("Shared casting montage loads"), Montage)
		|| !TestNotNull(TEXT("Native enemy animation instance exists"), Scene.Enemy->GetMesh()->GetAnimInstance()))
	{
		return false;
	}
	Scene.Enemy->SetTarget(Scene.Player);
	Scene.Stand();
	Scene.Enemy->GetCharacterMovement()->Velocity = FVector(100.0f, 0.0f, 0.0f);
	TestFalse(TEXT("Moving enemy cannot cast"), Scene.Enemy->CastSpell());
	Scene.Stand();
	TestTrue(TEXT("Stationary enemy starts the shared casting action"), Scene.Enemy->CastSpell());
	TestNull(TEXT("Enemy produces no projectile at montage start"), Scene.Spell());
	Scene.Advance(0.15f);
	TestNull(TEXT("Enemy produces no projectile before the release frame"), Scene.Spell());
	// Move the target after casting begins to check aiming at the actual release time.
	Scene.Player->SetActorLocation(FVector(600.0f, 150.0f, 96.0f));
	Scene.Advance(0.15f);
	ASpellProjectile* Spell = Scene.Spell();
	if (!TestNotNull(TEXT("Authored montage notify releases the enemy projectile"), Spell)) { return false; }
	TestEqual(TEXT("Enemy projectile records its caster"), Spell->GetOwner(), static_cast<AActor*>(Scene.Enemy));
	TestTrue(TEXT("Released projectile points toward the moved player"),
		Spell->GetActorForwardVector().Equals(
			(Scene.Player->GetActorLocation() - Spell->GetActorLocation()).GetSafeNormal(), 0.001f));
	TestFalse(TEXT("Enemy release notification is consumed once"), Scene.Enemy->ReleaseSpell(Montage));
	UHealthComponent* Health = Scene.Player->FindComponentByClass<UHealthComponent>();
	const float Before = Health->GetHealth();
	Spell->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.35f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Enemy spell deals ten damage to the player"), Health->GetHealth(), Before - 10.0f);
	TestTrue(TEXT("Enemy projectile is consumed on impact"), Spell->IsActorBeingDestroyed());
	Scene.Advance(0.6f);
	Scene.Stand();
	TestTrue(TEXT("Enemy can start another standing action"), Scene.Enemy->CastSpell());
	Scene.Advance(0.15f);
	Scene.Enemy->SetTarget(nullptr);
	TestFalse(TEXT("Lost target cancels an unreleased enemy action"), Scene.Enemy->IsCasting());
	TestFalse(TEXT("Lost target rejects a stale release notification"), Scene.Enemy->ReleaseSpell(Montage));
	Scene.Advance(0.4f);
	TestNull(TEXT("Losing the target before release produces no spell"), Scene.Spell());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDamageTest, "MagicTour.Enemy.PlayerDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDamageTest::RunTest(const FString& Parameters)
{
	FEnemyWorld Scene;
	if (!Scene.IsReady(*this)) { return false; }
	ASpellTestController* Controller = Scene.World->SpawnActor<ASpellTestController>();
	UHealthComponent* Health = Scene.Enemy->FindComponentByClass<UHealthComponent>();
	USkeletalMeshComponent* Mesh = Scene.Player->GetMesh();
	if (!TestNotNull(TEXT("Deterministic player aiming controller spawns"), Controller)
		|| !TestNotNull(TEXT("Enemy owns health"), Health)
		|| !TestNotNull(TEXT("Player animation instance exists"), Mesh->GetAnimInstance()))
	{
		return false;
	}

	// Aim through the enemy capsule without depending on a cached editor camera view.
	Scene.Player->SetActorLocation(FVector(-600.0f, 0.0f, 96.0f));
	Controller->Possess(Scene.Player);
	Controller->View = FVector(-900.0f, 0.0f, 96.0f);
	Controller->Rot = FRotator::ZeroRotator;
	Scene.Player->GetCharacterMovement()->StopMovementImmediately();
	Scene.Player->ConsumeMovementInputVector();
	Scene.Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	FindFProperty<FFloatProperty>(AMagicTourCharacter::StaticClass(), TEXT("Cooldown"))
		->SetPropertyValue_InContainer(Scene.Player, 0.0f);
	auto Advance = [&](float Delta)
	{
		Mesh->TickAnimation(Delta, false);
		Mesh->RefreshBoneTransforms();
		Mesh->AllowQueuedAnimEventsNextDispatch();
		Mesh->ConditionallyDispatchQueuedAnimEvents();
	};
	TestEqual(TEXT("Enemy starts with one hundred health"), Health->GetHealth(), 100.0f);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (!TestTrue(TEXT("Stationary player starts the real casting action"), Scene.Player->CastSpell()))
		{
			return false;
		}
		TestNull(TEXT("Player produces no projectile before the release notification"), Scene.Spell());
		Advance(0.3f);
		ASpellProjectile* Spell = Scene.Spell();
		if (!TestNotNull(TEXT("Player release notification emits a real projectile"), Spell)) { return false; }
		TestEqual(TEXT("Player projectile retains its caster"), Spell->GetOwner(), static_cast<AActor*>(Scene.Player));
		Spell->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.35f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Player projectile is consumed by the enemy impact"), Spell->IsActorBeingDestroyed());
		TestEqual(TEXT("Each player spell deals fifty damage to the enemy"),
			Health->GetHealth(), 100.0f - (Index + 1) * 50.0f);
		TestEqual(TEXT("Enemy dies on the second player impact"), Health->IsDead(), Index == 1);
		// Complete the current action before starting another cast in the same world frame.
		Advance(0.6f);
		TestFalse(TEXT("Player action finishes between impacts"), Scene.Player->IsCasting());
	}
	TestTrue(TEXT("Two player spells enter the existing enemy death state"),
		Scene.Enemy->GetState() == EEnemyState::Dead);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyDeathTest, "MagicTour.Enemy.Death",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathTest::RunTest(const FString& Parameters)
{
	FEnemyWorld Scene;
	if (!Scene.IsReady(*this)) { return false; }
	AEnemyCharacter* Other = Scene.SpawnEnemy(FVector(-600.0f, 0.0f, 96.0f));
	if (!TestNotNull(TEXT("Second enemy spawns"), Other)) { return false; }
	UHealthComponent* Health = Scene.Enemy->FindComponentByClass<UHealthComponent>();
	UHealthTestObserver* Probe = NewObject<UHealthTestObserver>();
	Health->OnDied.AddDynamic(Probe, &UHealthTestObserver::Died);
	const float Before = Health->GetHealth();
	TestEqual(TEXT("Enemy damage from another enemy is rejected"),
		UGameplayStatics::ApplyDamage(Scene.Enemy, 25.0f, Other->GetController(), Other, nullptr), 0.0f);
	TestEqual(TEXT("Friendly damage leaves enemy health unchanged"), Health->GetHealth(), Before);
	TestEqual(TEXT("Player damage can reduce enemy health"),
		UGameplayStatics::ApplyDamage(Scene.Enemy, 25.0f, Scene.Controller, Scene.Player, nullptr), 25.0f);
	Scene.Enemy->SetTarget(Scene.Player);
	Scene.Enemy->SetState(EEnemyState::Attack);
	Scene.Stand();
	TestTrue(TEXT("Enemy has a pending action before lethal damage"), Scene.Enemy->CastSpell());
	UGameplayStatics::ApplyDamage(Scene.Enemy, 1000.0f, Scene.Controller, Scene.Player, nullptr);
	TestTrue(TEXT("Player can defeat the enemy"), Health->IsDead());
	TestTrue(TEXT("Enemy death changes behavior state"), Scene.Enemy->GetState() == EEnemyState::Dead);
	TestFalse(TEXT("Death immediately interrupts the pending spell"), Scene.Enemy->IsCasting());
	TestFalse(TEXT("Dead enemy cannot start another spell"), Scene.Enemy->IsCastAllowed());
	TestTrue(TEXT("Enemy death stops movement"), Scene.Enemy->GetCharacterMovement()->MovementMode == MOVE_None);
	TestFalse(TEXT("Enemy death stops AI updates"), Scene.Enemy->GetController()->IsActorTickEnabled());
	Scene.Advance(0.4f);
	TestNull(TEXT("Dead enemy cannot release a pending projectile"), Scene.Spell());
	UGameplayStatics::ApplyDamage(Scene.Enemy, 1000.0f, Scene.Controller, Scene.Player, nullptr);
	TestEqual(TEXT("Enemy death is emitted only once"), Probe->Deaths, 1);
	Health->OnDied.RemoveAll(Probe);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuinsEnemySetupTest, "MagicTour.Enemy.RuinsSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRuinsEnemySetupTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Candidate = Context.World();
		if (Context.WorldType == EWorldType::Editor && Candidate
			&& Candidate->GetPackage()->GetName() == TEXT("/Game/ThirdPerson/Maps/Lvl_MagicTour"))
		{
			World = Candidate;
			break;
		}
	}
	if (!World)
	{
		AddInfo(TEXT("Ruins setup check skipped because Lvl_MagicTour is not open in the editor."));
		return true;
	}
	TArray<ACombatArena*> Arenas;
	TArray<AEnemyCharacter*> Enemies;
	for (TActorIterator<ACombatArena> It(World); It; ++It) { Arenas.Add(*It); }
	for (TActorIterator<AEnemyCharacter> It(World); It; ++It) { Enemies.Add(*It); }
	if (!TestEqual(TEXT("Map contains one combat arena"), Arenas.Num(), 1)
		|| !TestEqual(TEXT("Map contains exactly three enemies"), Enemies.Num(), 3)) { return false; }
	ACombatArena* Arena = Arenas[0];
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World);
	if (!TestNotNull(TEXT("Ruins map has a navigation system"), Nav)
		|| !TestNotNull(TEXT("Ruins navigation data is built"),
			Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))) { return false; }
	const FFloatProperty* Damage = FindFProperty<FFloatProperty>(AMagicTourCharacter::StaticClass(), TEXT("SpellDamage"));
	if (!TestNotNull(TEXT("Character exposes spell damage tuning"), Damage)) { return false; }
	for (AEnemyCharacter* Enemy : Enemies)
	{
		TestEqual(TEXT("Placed enemy references the common ruins arena"), Enemy->Arena.Get(), Arena);
		TestTrue(TEXT("Placed enemy starts inside the ruins"), Arena->IsInside(Enemy->GetActorLocation()));
		TestEqual(TEXT("Placed enemy spells deal ten damage"), Damage->GetPropertyValue_InContainer(Enemy), 10.0f);
		for (int32 Index = 0; Index < 12; ++Index)
		{
			FVector Pos;
			if (!TestTrue(TEXT("Ruins can produce a reachable patrol point"),
				Arena->IsPatrolFound(Enemy->GetActorLocation(), Pos))) { return false; }
			TestTrue(TEXT("Random patrol point remains inside the arena"), Arena->IsInside(Pos));
			UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
				World, Enemy->GetActorLocation(), Pos, Enemy);
			if (!TestNotNull(TEXT("Patrol path is generated"), Path)) { return false; }
			TestTrue(TEXT("Patrol path is valid and complete"), Path->IsValid() && !Path->IsPartial());
			TestTrue(TEXT("Entire patrol path remains inside the arena"), Arena->IsPathInside(Path->PathPoints));
		}
	}
	return true;
}

#endif
