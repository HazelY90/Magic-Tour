#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/MagicTourCharacter.h"
#include "Combat/SpellProjectile.h"
#include "Combat/CastNotify.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Components/FlightComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/SpellTestController.h"
#include "Tests/HealthTestObserver.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Own a complete gameplay world without touching the editor level. */
	struct FSpellWorld
	{
		UGameInstance* Instance;
		UWorld* World;
		FSpellWorld()
		{
			Instance = NewObject<UGameInstance>(GEngine);
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
			World->SetGameMode(FURL());
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}
		~FSpellWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			Instance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
		AMagicTourCharacter* Player(UClass* Class, const FVector& Pos)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<AMagicTourCharacter>(Class, Pos, FRotator::ZeroRotator, Params);
		}
		AActor* Wall(const FVector& Pos, const FVector& Extent)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
			Actor->SetRootComponent(Box);
			Box->SetBoxExtent(Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->RegisterComponent();
			Actor->SetActorLocation(Pos);
			return Actor;
		}
		ASpellProjectile* Spell()
		{
			for (TActorIterator<ASpellProjectile> It(World); It; ++It)
			{
				if (!It->IsActorBeingDestroyed()) { return *It; }
			}
			return nullptr;
		}
	};
	/** Tick once per engine frame so TimerManager can advance normally. */
	class FSpellExpiry : public IAutomationLatentCommand
	{
		FAutomationTestBase* Test;
		TSharedPtr<FSpellWorld> Scene;
		TWeakObjectPtr<ASpellProjectile> Spell;
		float Elapsed = 0.0f;
	public:
		FSpellExpiry(FAutomationTestBase* InTest, TSharedPtr<FSpellWorld> InScene, ASpellProjectile* InSpell)
			: Test(InTest), Scene(InScene), Spell(InSpell) {}
		virtual bool Update() override
		{
			Scene->World->Tick(LEVELTICK_All, 0.1f);
			Elapsed += 0.1f;
			const bool isExpired = !Spell.IsValid() || Spell->IsActorBeingDestroyed();
			if (isExpired || Elapsed >= 5.0f)
			{
				Test->TestTrue(TEXT("Missed projectile expires automatically"), isExpired);
				return true;
			}
			return false;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpellCastTest, "MagicTour.Spell.AimAndCast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpellCastTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	if (!TestNotNull(TEXT("Player Blueprint loads"), Class)) { return false; }
	FSpellWorld Scene;
	Scene.Wall(FVector(0.0f, 0.0f, -20.0f), FVector(3000.0f, 3000.0f, 20.0f));
	AMagicTourCharacter* Player = Scene.Player(Class, FVector(0.0f, 0.0f, 96.0f));
	ASpellTestController* Controller = Scene.World->SpawnActor<ASpellTestController>();
	if (!TestNotNull(TEXT("Player spawns"), Player) || !TestNotNull(TEXT("Controller spawns"), Controller)) { return false; }
	Controller->Possess(Player);
	// Keep aim geometry deterministic; action integration separately checks the real hand attachment.
	USceneComponent* Muzzle = nullptr;
	TArray<USceneComponent*> Parts;
	Player->GetComponents<USceneComponent>(Parts);
	for (USceneComponent* Part : Parts)
	{
		if (Part->GetFName() == TEXT("CastPoint")) { Muzzle = Part; break; }
	}
	if (!TestNotNull(TEXT("Muzzle exists"), Muzzle)) { return false; }
	Muzzle->AttachToComponent(Player->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	Muzzle->SetRelativeLocation(FVector(70.0f,35.0f,45.0f));
	UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr,TEXT("/Game/MagicTour/Characters/Animations/AM_Cast.AM_Cast"));
	UCharacterMovementComponent* Move = Player->GetCharacterMovement();
	UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
	Move->SetMovementMode(MOVE_Flying);
	TestFalse(TEXT("Flight forbids casting"), Player->CastSpell());
	Move->SetMovementMode(MOVE_Falling);
	TestFalse(TEXT("Falling forbids casting"), Player->CastSpell());
	// Without navigation data, SetMovementMode would fall back to walking.
	Move->MovementMode = MOVE_NavWalking;
	TestFalse(TEXT("Only MOVE_Walking is accepted"), Player->CastSpell());
	Move->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Living stationary walker can cast"), Player->IsCastAllowed());
	FVector Pos, Dir;
	TestTrue(TEXT("Unobstructed camera aim resolves"), Player->GetAim(Pos, Dir));
	const FVector FarTarget = Controller->View + Controller->Rot.Vector() * 10000.0f;
	TestTrue(TEXT("No-hit aim uses camera ray endpoint"), Dir.Equals((FarTarget - Pos).GetSafeNormal(), 0.001f));

	AActor* Target = Scene.Wall(FVector(500.0f, 0.0f, 150.0f), FVector(10.0f, 100.0f, 100.0f));
	Player->GetAim(Pos, Dir);
	TestTrue(TEXT("Camera hit converges from offset muzzle"), Dir.Equals((FVector(490.0f, 0.0f, 150.0f) - Pos).GetSafeNormal(), 0.001f));
	AActor* Near = Scene.Wall(FVector(45.0f, 20.0f, 120.0f), FVector(10.0f, 100.0f, 100.0f));
	Player->GetAim(Pos, Dir);
	TestTrue(TEXT("Near camera hit never launches backward"), FVector::DotProduct(Dir, Controller->Rot.Vector()) > 0.99f);
	TestTrue(TEXT("Blocked aim can start the action"), Player->CastSpell());
	TestFalse(TEXT("Wall between body and muzzle rejects notify spawn"), Player->ReleaseSpell(Montage));
	TestNull(TEXT("Blocked cast creates no projectile"), Scene.Spell());
	TestFalse(TEXT("Action prevents another cast"), Player->IsCastAllowed());
	Move->SetMovementMode(MOVE_Falling);
	Scene.World->Tick(LEVELTICK_All,0.5f);
	Move->SetMovementMode(MOVE_Walking);
	Near->Destroy();
	// This target is ahead of the muzzle but would otherwise cause a steep sideways shot.
	AActor* Close = Scene.Wall(FVector(Pos.X + 15.0f, 0.0f, 150.0f), FVector(2.0f, 10.0f, 10.0f));
	Player->GetAim(Pos, Dir);
	TestTrue(TEXT("Close lateral target uses camera-forward fallback"), Dir.Equals(Controller->Rot.Vector(), 0.001f));
	Close->Destroy();
	Target->Destroy();
	TestTrue(TEXT("Standing cast starts the action"), Player->CastSpell());
	TestNull(TEXT("Input does not spawn before the notification"), Scene.Spell());
	TestTrue(TEXT("Active montage notification spawns projectile"), Player->ReleaseSpell(Montage));
	TestFalse(TEXT("Duplicate notification cannot release twice"), Player->ReleaseSpell(Montage));
	ASpellProjectile* Spell = Scene.Spell();
	if (TestNotNull(TEXT("Spell exists"), Spell))
	{
		TestEqual(TEXT("Caster is the owner"), Spell->GetOwner(), static_cast<AActor*>(Player));
		TestEqual(TEXT("Caster is the instigator"), Spell->GetInstigator(), static_cast<APawn*>(Player));
		TestTrue(TEXT("Spell starts at muzzle"), Spell->GetActorLocation().Equals(Pos, 0.001f));
		TestFalse(TEXT("Cooldown prevents repeated cast"), Player->CastSpell());
		Spell->Destroy();
	}
	Scene.Wall(FVector(0.0f, 0.0f, -20.0f), FVector(3000.0f, 3000.0f, 20.0f));
	Move->SetMovementMode(MOVE_Falling);
	Scene.World->Tick(LEVELTICK_All, 0.8f);
	Move->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Cooldown expires using world time"), Player->IsCastAllowed());
	if (Health)
	{
		Health->ApplyDamage(Health->GetMaxHealth());
		TestFalse(TEXT("Dead walker cannot cast"), Player->CastSpell());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpellImpactTest, "MagicTour.Spell.ImpactAndLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpellImpactTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	if (!TestNotNull(TEXT("Player Blueprint loads"), Class)) { return false; }
	FSpellWorld Scene;
	Scene.Wall(FVector(0.0f, 0.0f, -20.0f), FVector(3000.0f, 3000.0f, 20.0f));
	AMagicTourCharacter* Caster = Scene.Player(Class, FVector(0.0f, 0.0f, 96.0f));
	AMagicTourCharacter* Target = Scene.Player(Class, FVector(500.0f, 0.0f, 96.0f));
	if (!TestNotNull(TEXT("Caster spawns"), Caster) || !TestNotNull(TEXT("Damage target spawns"), Target)) { return false; }
	UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
	UHealthComponent* CasterHealth = Caster->FindComponentByClass<UHealthComponent>();
	if (!TestNotNull(TEXT("Target has health"), Health)) { return false; }

	auto Fire = [&](const FVector& Pos)
	{
		const FTransform Transform(FRotator::ZeroRotator, Pos);
		ASpellProjectile* Spell = Scene.World->SpawnActorDeferred<ASpellProjectile>(ASpellProjectile::StaticClass(),
			Transform, Caster, Caster, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Spell->Init(Caster, FVector::ForwardVector, 25.0f);
		Spell->FinishSpawning(Transform);
		return Spell;
	};
	const float StartHealth = Health->GetHealth();
	ASpellProjectile* Spell = Fire(FVector(0.0f, 0.0f, 96.0f));
	UProjectileMovementComponent* Move = Spell->FindComponentByClass<UProjectileMovementComponent>();
	TestEqual(TEXT("Magic has no gravity"), Move->ProjectileGravityScale, 0.0f);
	Move->TickComponent(0.3f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Swept impact applies exactly one damage amount"), Health->GetHealth(), StartHealth - 25.0f);
	TestTrue(TEXT("Impact destroys the projectile"), Spell->IsActorBeingDestroyed());
	TestEqual(TEXT("Caster collision is ignored"), CasterHealth->GetHealth(), CasterHealth->GetMaxHealth());
	Scene.World->Tick(LEVELTICK_All, 0.05f);
	TestEqual(TEXT("Later world update cannot apply damage twice"), Health->GetHealth(), StartHealth - 25.0f);

	AActor* Wall = Scene.Wall(FVector(250.0f, 0.0f, 96.0f), FVector(10.0f, 100.0f, 100.0f));
	ASpellProjectile* Blocked = Fire(FVector(100.0f, 0.0f, 96.0f));
	Blocked->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.3f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Obstacle destroys the projectile"), Blocked->IsActorBeingDestroyed());
	TestEqual(TEXT("Obstacle prevents damage behind it"), Health->GetHealth(), StartHealth - 25.0f);
	Wall->Destroy();
	UHealthTestObserver* Probe = NewObject<UHealthTestObserver>();
	Health->OnDied.AddDynamic(Probe, &UHealthTestObserver::Died);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		ASpellProjectile* Shot = Fire(FVector(100.0f, 0.0f, 96.0f));
		Shot->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.3f, LEVELTICK_All, nullptr);
	}
	TestEqual(TEXT("Repeated spells reduce health to zero"), Health->GetHealth(), 0.0f);
	TestEqual(TEXT("Spell damage emits one death"), Probe->Deaths, 1);
	ASpellProjectile* Extra = Fire(FVector(100.0f, 0.0f, 96.0f));
	Extra->FindComponentByClass<UProjectileMovementComponent>()->TickComponent(0.3f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Hitting a dead target cannot repeat death"), Probe->Deaths, 1);
	Health->OnDied.RemoveAll(Probe);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpellLifetimeTest, "MagicTour.Spell.Lifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpellLifetimeTest::RunTest(const FString& Parameters)
{
	TSharedPtr<FSpellWorld> Scene = MakeShared<FSpellWorld>();
	ASpellProjectile* Empty = Scene->World->SpawnActor<ASpellProjectile>(
		ASpellProjectile::StaticClass(), FVector(0.0f, 1000.0f, 1000.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Missed projectile spawns"), Empty)) { return false; }
	TestTrue(TEXT("Missed projectile has a finite lifetime"), Empty->GetLifeSpan() > 0.0f);
	ADD_LATENT_AUTOMATION_COMMAND(FSpellExpiry(this, Scene, Empty));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpellDirectionTest, "MagicTour.Spell.WorldDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpellDirectionTest::RunTest(const FString& Parameters)
{
	FSpellWorld Scene;
	const TArray<FRotator> Angles = {
		FRotator(0, 0, 0), FRotator(0, 45, 0), FRotator(0, -45, 0),
		FRotator(0, 90, 0), FRotator(0, -90, 0), FRotator(0, 135, 0),
		FRotator(0, 180, 0), FRotator(0, -135, 0),
		FRotator(60, 45, 0), FRotator(-60, -90, 0), FRotator(80, 135, 0)
	};
	for (const FRotator& Angle : Angles)
	{
		const FVector Dir = Angle.Vector();
		const FVector Pos(0.0f, 0.0f, 1000.0f);
		const FTransform Transform(Angle, Pos);
		ASpellProjectile* Spell = Scene.World->SpawnActorDeferred<ASpellProjectile>(
			ASpellProjectile::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!TestNotNull(TEXT("Directional projectile spawns"), Spell)) { return false; }
		Spell->Init(nullptr, Dir, 25.0f);
		Spell->FinishSpawning(Transform);
		UProjectileMovementComponent* Move = Spell->FindComponentByClass<UProjectileMovementComponent>();
		const FString Label = Angle.ToString();
		TestFalse(*FString::Printf(TEXT("World-space velocity: %s"), *Label), Move->bInitialVelocityInLocalSpace);
		TestTrue(*FString::Printf(TEXT("Launch velocity matches aim: %s"), *Label), Move->Velocity.GetSafeNormal().Equals(Dir, 0.001f));
		TestTrue(*FString::Printf(TEXT("Visual orientation matches aim: %s"), *Label), Spell->GetActorForwardVector().Equals(Dir, 0.001f));
		Move->TickComponent(0.05f, LEVELTICK_All, nullptr);
		TestTrue(*FString::Printf(TEXT("Actual flight matches aim: %s"), *Label),
			(Spell->GetActorLocation() - Pos).GetSafeNormal().Equals(Dir, 0.001f));
		Spell->Destroy();
	}
	// Init must remain authoritative even if the spawn transform differs from the aim.
	const FVector Dir = FRotator(30, -90, 0).Vector();
	const FTransform Transform(FRotator(0, 45, 0), FVector(0, 0, 1000));
	ASpellProjectile* Spell = Scene.World->SpawnActorDeferred<ASpellProjectile>(
		ASpellProjectile::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Spell->Init(nullptr, Dir, 25.0f);
	Spell->FinishSpawning(Transform);
	TestTrue(TEXT("Stored aim survives a different spawn rotation"),
		Spell->FindComponentByClass<UProjectileMovementComponent>()->Velocity.GetSafeNormal().Equals(Dir, 0.001f));
	Spell->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCastActionTest, "MagicTour.Spell.CastAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCastActionTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr,
		TEXT("/Game/MagicTour/Characters/Animations/AM_Cast.AM_Cast"));
	if (!TestNotNull(TEXT("Player Blueprint loads"),Class) || !TestNotNull(TEXT("Action montage loads"),Montage)) { return false; }
	FSpellWorld Scene;
	Scene.Wall(FVector(0,0,-20),FVector(3000,3000,20));
	AMagicTourCharacter* Player = Scene.Player(Class,FVector(0,0,96));
	ASpellTestController* Controller = Scene.World->SpawnActor<ASpellTestController>();
	Controller->Possess(Player);
	UCharacterMovementComponent* Move = Player->GetCharacterMovement();
	USkeletalMeshComponent* Mesh = Player->GetMesh();
	UAnimInstance* Anim = Mesh->GetAnimInstance();
	if (!TestNotNull(TEXT("Animation instance exists"),Anim)) { return false; }
	// Remove cooldown only in this fixture to isolate interruption and notification behavior.
	FindFProperty<FFloatProperty>(AMagicTourCharacter::StaticClass(),TEXT("Cooldown"))->SetPropertyValue_InContainer(Player,0.0f);
	TArray<USceneComponent*> Parts;
	Player->GetComponents<USceneComponent>(Parts);
	for (USceneComponent* Part : Parts)
	{
		if (Part->GetFName() == TEXT("CastPoint"))
		{
			TestEqual(TEXT("Muzzle follows the skeletal mesh"),Part->GetAttachParent(),static_cast<USceneComponent*>(Mesh));
			TestEqual(TEXT("Muzzle attaches to the casting hand"),Part->GetAttachSocketName(),FName(TEXT("hand_r")));
		}
	}
	auto Advance = [&](float Delta)
	{
		Mesh->TickAnimation(Delta,false);
		Mesh->RefreshBoneTransforms();
		Mesh->AllowQueuedAnimEventsNextDispatch();
		Mesh->ConditionallyDispatchQueuedAnimEvents();
	};
	Move->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Standing starts the montage"),Player->CastSpell());
	TestTrue(TEXT("Action holds a pending cast"),Player->IsCasting());
	TestFalse(TEXT("Repeated input cannot start another action"),Player->CastSpell());
	TestNull(TEXT("No immediate projectile"),Scene.Spell());
	Advance(0.15f);
	TestNull(TEXT("No projectile during arm raise"),Scene.Spell());
	Advance(0.15f);
	ASpellProjectile* Released = Scene.Spell();
	TestNotNull(TEXT("Authored notify releases a projectile at the raised-hand frame"),Released);
	TestFalse(TEXT("Duplicate authored notify is consumed"),Player->ReleaseSpell(Montage));
	if (Released) { Released->Destroy(); }
	Advance(0.6f);
	TestFalse(TEXT("Completed montage clears casting state"),Player->IsCasting());
	TestTrue(TEXT("Completed montage restores movement orientation"),Move->bOrientRotationToMovement);

	TestTrue(TEXT("Action before rapid restart starts"),Player->CastSpell());
	const int32 OldInstance = Anim->GetActiveInstanceForMontage(Montage)->GetInstanceID();
	Move->SetMovementMode(MOVE_Flying);
	Move->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Action can restart after cancellation"),Player->CastSpell());
	TestTrue(TEXT("Old end callback does not cancel the restarted action"),Player->IsCasting());
	TestFalse(TEXT("Old instance notify cannot release the restarted action"),Player->ReleaseSpell(Montage,OldInstance));
	Advance(0.3f);
	ASpellProjectile* Restarted = Scene.Spell();
	TestNotNull(TEXT("Restarted instance releases normally"),Restarted);
	if (Restarted) { Restarted->Destroy(); }
	Advance(0.6f);

	for (EMovementMode Mode : {MOVE_Flying,MOVE_Falling})
	{
		Move->SetMovementMode(MOVE_Walking);
		TestTrue(TEXT("Next action starts"),Player->CastSpell());
		Move->SetMovementMode(Mode);
		TestFalse(TEXT("Movement transition immediately cancels action"),Player->IsCasting());
		TestFalse(TEXT("Canceled action rejects stale notify"),Player->ReleaseSpell(Montage));
		Advance(0.4f);
		TestNull(TEXT("Canceled action produces no projectile"),Scene.Spell());
	}
	Move->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Action before interruption starts"),Player->CastSpell());
	Anim->Montage_Stop(0.0f,Montage);
	Advance(0.1f);
	TestFalse(TEXT("Interrupted montage clears action"),Player->IsCasting());
	TestFalse(TEXT("Interrupted montage rejects late notify"),Player->ReleaseSpell(Montage));
	TestTrue(TEXT("Action before death starts"),Player->CastSpell());
	Player->FindComponentByClass<UHealthComponent>()->ApplyDamage(1000.0f);
	TestFalse(TEXT("Death immediately cancels action"),Player->IsCasting());
	TestFalse(TEXT("Death rejects late notify"),Player->ReleaseSpell(Montage));
	Advance(0.4f);
	TestNull(TEXT("Dead character produces no projectile"),Scene.Spell());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStandingCastTest, "MagicTour.Spell.StandingCast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStandingCastTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr,
		TEXT("/Game/MagicTour/Characters/Animations/AM_Cast.AM_Cast"));
	if (!TestNotNull(TEXT("Player Blueprint loads"), Class)
		|| !TestNotNull(TEXT("Action montage loads"), Montage)) { return false; }
	FSpellWorld Scene;
	Scene.Wall(FVector(0,0,-20), FVector(3000,3000,20));
	AMagicTourCharacter* Player = Scene.Player(Class, FVector(0,0,96));
	ASpellTestController* Controller = Scene.World->SpawnActor<ASpellTestController>();
	if (!TestNotNull(TEXT("Player spawns"), Player)
		|| !TestNotNull(TEXT("Controller spawns"), Controller)) { return false; }
	Controller->Possess(Player);
	UCharacterMovementComponent* Move = Player->GetCharacterMovement();
	USkeletalMeshComponent* Mesh = Player->GetMesh();
	UAnimInstance* Anim = Mesh->GetAnimInstance();
	if (!TestNotNull(TEXT("Animation instance exists"), Anim)) { return false; }
	FindFProperty<FFloatProperty>(AMagicTourCharacter::StaticClass(), TEXT("Cooldown"))
		->SetPropertyValue_InContainer(Player, 0.0f);
	auto Advance = [&](float Delta)
	{
		Mesh->TickAnimation(Delta, false);
		Mesh->RefreshBoneTransforms();
		Mesh->AllowQueuedAnimEventsNextDispatch();
		Mesh->ConditionallyDispatchQueuedAnimEvents();
	};
	auto Input = [&](float Right, float Forward)
	{
		// Exercise the same reflected movement entry point used by controls and Blueprint UI.
		struct { float Right; float Forward; } Args{Right, Forward};
		Player->ProcessEvent(Player->FindFunctionChecked(TEXT("DoMove")), &Args);
	};
	Move->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Standing permits a new cast"), Player->IsCastAllowed());
	Move->Velocity = FVector(100.0f, 0.0f, 0.0f);
	TestFalse(TEXT("Ground motion forbids a new cast"), Player->CastSpell());
	Move->Velocity = FVector::ZeroVector;
	Input(0.0f, 1.0f);
	TestFalse(TEXT("Queued input forbids casting before movement ticks"), Player->CastSpell());
	Player->ConsumeMovementInputVector();
	TestTrue(TEXT("Standing again permits casting"), Player->CastSpell());
	Input(0.0f, 0.0f);
	Controller->SetControlRotation(FRotator(0.0f, 30.0f, 0.0f));
	TestTrue(TEXT("Zero movement input and aiming preserve the cast"), Player->IsCasting());
	Advance(0.15f);
	Input(1.0f, 0.0f);
	TestFalse(TEXT("Movement input immediately interrupts before release"), Player->IsCasting());
	TestTrue(TEXT("Interruption restores movement orientation"), Move->bOrientRotationToMovement);
	TestFalse(TEXT("Interrupted action rejects a late release"), Player->ReleaseSpell(Montage));
	Player->ConsumeMovementInputVector();
	Advance(0.4f);
	TestNull(TEXT("Interrupted action produces no projectile"), Scene.Spell());

	TestTrue(TEXT("Action can restart after input cancellation"), Player->CastSpell());
	// Drive actual CharacterMovement to cover motion originating outside DoMove.
	Player->AddMovementInput(FVector::ForwardVector, 1.0f);
	Move->TickComponent(0.05f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Movement update interrupts external movement input"), Player->IsCasting());
	Move->Velocity = FVector::ZeroVector;
	TestFalse(TEXT("Acceleration forbids casting even at zero speed"), Player->IsCastAllowed());
	Advance(0.4f);
	TestNull(TEXT("External movement produces no projectile"), Scene.Spell());
	Move->StopMovementImmediately();
	Player->ConsumeMovementInputVector();
	Move->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Stopped character can restart"), Player->CastSpell());
	Move->Velocity = FVector(100.0f, 0.0f, 0.0f);
	TestFalse(TEXT("Release rechecks motion before spawning"), Player->ReleaseSpell(Montage));
	TestFalse(TEXT("Moving at release cancels the action"), Player->IsCasting());
	Move->StopMovementImmediately();
	Advance(0.4f);
	TestNull(TEXT("Release-time cancellation produces no projectile"), Scene.Spell());

	TestTrue(TEXT("Final stationary action starts"), Player->CastSpell());
	Advance(0.3f);
	ASpellProjectile* Spell = Scene.Spell();
	if (TestNotNull(TEXT("Stationary action releases normally"), Spell))
	{
		Input(0.0f, 1.0f);
		Move->TickComponent(0.05f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Movement after release preserves the action"), Player->IsCasting());
		TestFalse(TEXT("Movement cannot revoke the released projectile"), Spell->IsActorBeingDestroyed());
		TestFalse(TEXT("Released action still rejects duplicate notifications"), Player->ReleaseSpell(Montage));
		Spell->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDeathTest, "MagicTour.Health.PlayerDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDeathTest::RunTest(const FString& Parameters)
{
	FSpellWorld Scene;
	Scene.Wall(FVector(0.0f, 0.0f, -20.0f), FVector(3000.0f, 3000.0f, 20.0f));
	UClass* Class = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	if (!TestNotNull(TEXT("Player Blueprint loads"), Class)) { return false; }
	for (int32 Case = 0; Case < 2; ++Case)
	{
		// Cover lethal damage during a pending ground cast and during automatic takeoff.
		AMagicTourCharacter* Player = Scene.Player(Class, FVector(Case * 600.0f, 0.0f, 96.0f));
		APlayerController* Controller = Scene.World->SpawnActor<APlayerController>();
		if (!TestNotNull(TEXT("Player spawns"), Player)
			|| !TestNotNull(TEXT("Player controller spawns"), Controller)) { return false; }
		Controller->Possess(Player);
		UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
		UFlightComponent* Flight = Player->FindComponentByClass<UFlightComponent>();
		UCharacterMovementComponent* Move = Player->GetCharacterMovement();
		Move->SetMovementMode(MOVE_Walking);
		if (Case == 0) { TestTrue(TEXT("Ground cast starts before death"), Player->CastSpell()); }
		else
		{
			Flight->ToggleFlight();
			Player->SetActorLocation(FVector(600.0f, 0.0f, 900.0f));
			TestTrue(TEXT("Takeoff is active before death"), Flight->IsTakingOff());
		}
		Health->ApplyDamage(1000.0f);
		TestTrue(TEXT("Lethal damage stops player movement"), Move->MovementMode == MOVE_None);
		TestTrue(TEXT("Lethal damage clears velocity"), Move->Velocity.IsNearlyZero());
		TestFalse(TEXT("Death cancels casting"), Player->IsCasting());
		TestFalse(TEXT("Dead player cannot cast"), Player->CastSpell());
		TestFalse(TEXT("Death exits flight"), Flight->IsFlying());
		TestFalse(TEXT("Death stops flight ticking"), Flight->IsComponentTickEnabled());
		TestTrue(TEXT("Body is placed on the floor"), FMath::IsNearlyEqual(Player->GetActorLocation().Z, 96.0f, 0.1f));
		TestTrue(TEXT("Body does not block movement"), Player->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestEqual(TEXT("Player body is retained"), Player->GetLifeSpan(), 0.0f);
		USkeletalMeshComponent* Mesh = Player->GetMesh();
		UAnimSingleNodeInstance* Anim = Mesh->GetSingleNodeInstance();
		if (TestNotNull(TEXT("Death replaces locomotion with a fixed pose"), Anim))
		{
			TestNotNull(TEXT("Death pose is loaded"), Anim->GetCurrentAsset());
			Mesh->TickAnimation(0.1f, false);
			Mesh->RefreshBoneTransforms();
			const FVector Head = Mesh->GetBoneLocation(TEXT("head"));
			const FVector Hips = Mesh->GetBoneLocation(TEXT("pelvis"));
			AddInfo(FString::Printf(TEXT("Death head %s, pelvis %s, root %s"),
				*Head.ToString(), *Hips.ToString(), *Mesh->GetBoneQuaternion(TEXT("root")).Rotator().ToString()));
			TestTrue(TEXT("Fixed body is horizontal"), FMath::Abs(Head.Z - Hips.Z) < 40.0f);
			TestEqual(TEXT("Fixed pose does not advance"), Anim->GetCurrentTime(), 0.0f);
		}
		const FVector Pos = Player->GetActorLocation();
		Health->ApplyDamage(1000.0f);
		Scene.World->Tick(LEVELTICK_All, 0.1f);
		TestTrue(TEXT("Repeated lethal damage retains the same body location"), Player->GetActorLocation().Equals(Pos, 0.1f));
		TestNull(TEXT("Interrupted cast cannot release a projectile after death"), Scene.Spell());
		Player->Destroy();
		Controller->Destroy();
	}
	return true;
}

#endif
