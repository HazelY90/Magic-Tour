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
		const float DefaultAccel = Move->MaxAcceleration;
		TestFalse(TEXT("Character starts outside flight"), Flight->IsFlying());
		TestFalse(TEXT("Broom starts hidden"), Broom->IsVisible());

		Flight->ToggleFlight();
		TestTrue(TEXT("Toggle enters MOVE_Flying"), Flight->IsFlying());
		TestTrue(TEXT("Toggle starts automatic takeoff"), Flight->IsTakingOff());
		TestEqual(TEXT("Flight uses walking speed"), Move->MaxFlySpeed, Move->MaxWalkSpeed);
		TestEqual(TEXT("Flight uses walking braking"), Move->BrakingDecelerationFlying, Move->BrakingDecelerationWalking);
		TestEqual(TEXT("Flight keeps default acceleration"), Move->MaxAcceleration, DefaultAccel);
		TestTrue(TEXT("Broom is visible during flight"), Broom->IsVisible());

		Flight->TickComponent(0.1f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Automatic takeoff adds upward velocity"), Move->Velocity.Z > 0.0f);
		Character->SetActorLocation(FVector(0.0f, 0.0f, 200.0f));
		Flight->TickComponent(0.1f, LEVELTICK_All, nullptr);
		TestFalse(TEXT("Automatic takeoff ends at target height"), Flight->IsTakingOff());
		TestEqual(TEXT("Automatic takeoff stops vertical velocity"), Move->Velocity.Z, 0.0);

		Flight->MoveVertical(1.0f);
		TestTrue(TEXT("Ascend adds positive world-Z input"), Character->GetPendingMovementInputVector().Z > 0.0f);
		Character->ConsumeMovementInputVector();
		Flight->MoveVertical(-1.0f);
		TestTrue(TEXT("Descend is blocked at the flight floor"), Character->GetPendingMovementInputVector().IsNearlyZero());
		Character->SetActorLocation(FVector(0.0f, 0.0f, 250.0f));
		Flight->MoveVertical(-1.0f);
		TestTrue(TEXT("Descend works above the flight floor"), Character->GetPendingMovementInputVector().Z < 0.0f);
		Character->ConsumeMovementInputVector();
		Character->SetActorLocation(FVector(0.0f, 0.0f, 190.0f));
		Move->Velocity.Z = -100.0f;
		Flight->TickComponent(0.1f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Flight floor clamps altitude"), Character->GetActorLocation().Z >= 200.0f);
		TestEqual(TEXT("Flight floor stops downward velocity"), Move->Velocity.Z, 0.0);

		Flight->ToggleFlight();
		TestFalse(TEXT("Second toggle leaves flight"), Flight->IsFlying());
		TestEqual(TEXT("Flight exit enables gravity movement"), Move->MovementMode, MOVE_Falling);
		TestTrue(TEXT("Flight exit clears residual velocity"), Move->Velocity.IsNearlyZero());
		TestEqual(TEXT("Flight exit keeps default acceleration"), Move->MaxAcceleration, DefaultAccel);
		TestFalse(TEXT("Broom hides after flight"), Broom->IsVisible());
	}

	Character->Destroy();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
