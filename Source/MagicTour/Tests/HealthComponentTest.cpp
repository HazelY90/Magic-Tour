#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/MagicTourCharacter.h"
#include "Components/HealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Tests/HealthTestObserver.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHealthComponentTest, "MagicTour.Health.Damage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHealthComponentTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<AMagicTourCharacter>(nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	if (!TestNotNull(TEXT("Player Blueprint loads"), Class))
	{
		return false;
	}

	// Use an isolated world to exercise the actual UE damage path.
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	// Pawn damage requires an authoritative GameMode, as it does in PIE.
	World->SetGameMode(FURL());
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMagicTourCharacter* Player = World->SpawnActor<AMagicTourCharacter>(Class, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	// Exercise a non-default Blueprint capacity before gameplay initialization.
	if (Health)
	{
		FFloatProperty* MaxProp = FindFProperty<FFloatProperty>(UHealthComponent::StaticClass(), TEXT("Max"));
		if (TestNotNull(TEXT("Capacity is configurable"), MaxProp))
		{
			MaxProp->SetPropertyValue_InContainer(Health, 200.0f);
		}
	}
	World->BeginPlay();
	if (TestNotNull(TEXT("Player owns health"), Health))
	{
		UHealthTestObserver* Probe = NewObject<UHealthTestObserver>();
		Probe->Health = Health;
		Health->OnHealthChanged.AddDynamic(Probe, &UHealthTestObserver::Changed);
		Health->OnDied.AddDynamic(Probe, &UHealthTestObserver::Died);
		TestEqual(TEXT("Configured capacity survives initialization"), Health->GetMaxHealth(), 200.0f);
		TestEqual(TEXT("Starts at configured capacity"), Health->GetHealth(), Health->GetMaxHealth());
		TestEqual(TEXT("Starts with full ratio"), Health->GetRatio(), 1.0f);
		TestFalse(TEXT("Starts alive"), Health->IsDead());
		TestEqual(TEXT("Negative damage ignored"), Health->ApplyDamage(-10.0f), 0.0f);
		TestEqual(TEXT("Zero damage ignored"), Health->ApplyDamage(0.0f), 0.0f);
		TestEqual(TEXT("NaN ignored"), Health->ApplyDamage(std::numeric_limits<float>::quiet_NaN()), 0.0f);
		TestEqual(TEXT("Infinity ignored"), Health->ApplyDamage(std::numeric_limits<float>::infinity()), 0.0f);
		TestEqual(TEXT("Invalid damage emits no changes"), Probe->Changes, 0);
		Player->SetCanBeDamaged(false);
		TestEqual(TEXT("Invulnerable player rejects UE damage"), UGameplayStatics::ApplyDamage(Player, 10.0f, nullptr, nullptr, nullptr), 0.0f);
		Player->SetCanBeDamaged(true);
		const float Amount = Health->GetMaxHealth() * 0.25f;
		TestEqual(TEXT("UE damage returns actual loss"), UGameplayStatics::ApplyDamage(Player, Amount, nullptr, nullptr, nullptr), Amount);
		TestEqual(TEXT("Damage updates ratio"), Health->GetRatio(), 0.75f);
		TestEqual(TEXT("Damage emits one change"), Probe->Changes, 1);
		TestEqual(TEXT("Nonlethal damage emits no death"), Probe->Deaths, 0);
		const float Remaining = Health->GetHealth();
		Probe->isReentrant = true;
		TestEqual(TEXT("Overkill returns remaining health"), UGameplayStatics::ApplyDamage(Player, Health->GetMaxHealth() * 2.0f, nullptr, nullptr, nullptr), Remaining);
		TestEqual(TEXT("Health clamps at zero"), Health->GetHealth(), 0.0f);
		TestTrue(TEXT("Zero health is dead"), Health->IsDead());
		TestEqual(TEXT("Reentrant lethal damage emits one death"), Probe->Deaths, 1);
		TestEqual(TEXT("Repeated damage is ignored"), UGameplayStatics::ApplyDamage(Player, 10.0f, nullptr, nullptr, nullptr), 0.0f);
		TestEqual(TEXT("Only effective damage emits changes"), Probe->Changes, 2);
		TestEqual(TEXT("Death stays single"), Probe->Deaths, 1);
		Health->OnHealthChanged.RemoveAll(Probe);
		Health->OnDied.RemoveAll(Probe);
	}
	Instance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
