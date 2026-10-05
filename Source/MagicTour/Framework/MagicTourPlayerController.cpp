// Copyright Epic Games, Inc. All Rights Reserved.


#include "Framework/MagicTourPlayerController.h"
#include "Framework/MagicTourGameMode.h"
#include "Components/HealthComponent.h"
#include "Characters/MagicTourCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Framework/MagicTourCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "MagicTour.h"
#include "UI/MagicTourHUDWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

AMagicTourPlayerController::AMagicTourPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AMagicTourCameraManager::StaticClass();
}

void AMagicTourPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Create the persistent gameplay HUD before optional touch controls.
	HUDWidget = CreateWidget<UMagicTourHUDWidget>(this, UMagicTourHUDWidget::StaticClass());
	if (HUDWidget)
	{
		HUDWidget->SetProgress(GetWorld() ? GetWorld()->GetAuthGameMode<AMagicTourGameMode>() : nullptr);
		HUDWidget->AddToPlayerScreen(10);
	}
	else
	{
		UE_LOG(LogMagicTour, Error, TEXT("Could not spawn the gameplay HUD widget."));
	}

	OnPossessedPawnChanged.AddUniqueDynamic(this, &AMagicTourPlayerController::PawnChanged);
	PawnChanged(nullptr, GetPawn());

	// Only spawn touch controls when the current platform requires them.
	if (ShouldUseTouchControls())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogMagicTour, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void AMagicTourPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
			{
				CastAction = NewObject<UInputAction>(this);
				CastAction->ValueType = EInputActionValueType::Boolean;
				CastMap = NewObject<UInputMappingContext>(this);
				CastMap->MapKey(CastAction, EKeys::LeftMouseButton);
				CastMap->MapKey(CastAction, EKeys::Gamepad_RightTrigger);
				Subsystem->AddMappingContext(CastMap, 0);
				Input->BindAction(CastAction, ETriggerEvent::Started, this, &AMagicTourPlayerController::CastInput);
			}

			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

bool AMagicTourPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AMagicTourPlayerController::PawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (Health)
	{
		Health->OnHealthChanged.RemoveDynamic(this, &AMagicTourPlayerController::HealthChanged);
	}

	Health = NewPawn ? NewPawn->FindComponentByClass<UHealthComponent>() : nullptr;
	if (Health)
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &AMagicTourPlayerController::HealthChanged);
	}

	// Synchronize immediately; possession can happen after component BeginPlay.
	HealthChanged(Health ? Health->GetHealth() : 0.0f, Health ? Health->GetMaxHealth() : 0.0f);
}

void AMagicTourPlayerController::HealthChanged(float Current, float Max)
{
	if (HUDWidget)
	{
		HUDWidget->SetHealth(Max > 0.0f ? Current / Max : 0.0f);
	}
}

void AMagicTourPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (CastMap)
		{
			Subsystem->RemoveMappingContext(CastMap);
		}
	}
	OnPossessedPawnChanged.RemoveDynamic(this, &AMagicTourPlayerController::PawnChanged);
	PawnChanged(GetPawn(), nullptr);
	Super::EndPlay(Reason);
}

void AMagicTourPlayerController::CastInput()
{
	if (AMagicTourCharacter* Character = Cast<AMagicTourCharacter>(GetPawn()))
	{
		Character->CastSpell();
	}
}
