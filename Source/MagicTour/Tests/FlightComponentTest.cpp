// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/MagicTourCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/FlightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlightComponentTest,
	"MagicTour.Flight.BasicMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlightComponentTest::RunTest(const FString& Parameters)
{
	UClass* CharacterClass = LoadClass<AMagicTourCharacter>(
		nullptr,
		TEXT("/Game/ThirdPerson/Blueprints/BP_MagicTourCharacter.BP_MagicTourCharacter_C"));
	if (!TestNotNull(TEXT("Character Blueprint loads"), CharacterClass))
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMagicTourCharacter* Character = World->SpawnActor<AMagicTourCharacter>(CharacterClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Character spawns"), Character))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();
	if (!Character->HasActorBegunPlay())
	{
		Character->DispatchBeginPlay();
	}

	UFlightComponent* Flight = Character->FindComponentByClass<UFlightComponent>();
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	USceneComponent* Broom = nullptr;
	TArray<USceneComponent*> Scenes;
	Character->GetComponents<USceneComponent>(Scenes);
	for (USceneComponent* Scene : Scenes)
	{
		if (Scene && Scene->GetFName() == TEXT("BroomRoot"))
		{
			Broom = Scene;
			break;
		}
	}

	TestNotNull(TEXT("Flight component exists"), Flight);
	TestNotNull(TEXT("Movement component exists"), Move);
	TestNotNull(TEXT("Broom root exists"), Broom);
	TestTrue(TEXT("Character BeginPlay runs"), Character->HasActorBegunPlay());
	TestTrue(TEXT("Capsule collision is enabled"), Character->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision);

	if (Flight && Move && Broom)
	{
		TestFalse(TEXT("Character starts outside flight"), Flight->IsFlying());
		TestFalse(TEXT("Broom starts hidden"), Broom->IsVisible());

		Flight->ToggleFlight();
		TestTrue(TEXT("Toggle enters MOVE_Flying"), Flight->IsFlying());
		TestEqual(TEXT("Normal flight speed"), Move->MaxFlySpeed, 900.0f);
		TestTrue(TEXT("Broom is visible during flight"), Broom->IsVisible());

		Flight->MoveVertical(1.0f);
		TestTrue(TEXT("Ascend adds positive world-Z input"), Character->GetPendingMovementInputVector().Z > 0.0f);
		Character->ConsumeMovementInputVector();
		Flight->MoveVertical(-1.0f);
		TestTrue(TEXT("Descend adds negative world-Z input"), Character->GetPendingMovementInputVector().Z < 0.0f);

		Flight->StartBoost();
		TestEqual(TEXT("Boost speed"), Move->MaxFlySpeed, 1600.0f);
		Flight->StopBoost();
		TestEqual(TEXT("Boost release restores normal speed"), Move->MaxFlySpeed, 900.0f);

		Flight->ToggleFlight();
		TestFalse(TEXT("Second toggle leaves flight"), Flight->IsFlying());
		TestEqual(TEXT("Flight exit enables gravity movement"), Move->MovementMode, MOVE_Falling);
		TestFalse(TEXT("Broom hides after flight"), Broom->IsVisible());
	}

	Character->Destroy();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
