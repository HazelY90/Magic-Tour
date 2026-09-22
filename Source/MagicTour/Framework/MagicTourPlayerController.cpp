// Copyright Epic Games, Inc. All Rights Reserved.


#include "Framework/MagicTourPlayerController.h"
#include "Framework/MagicTourGameMode.h"
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
