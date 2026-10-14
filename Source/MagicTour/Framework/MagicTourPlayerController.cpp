// Copyright Epic Games, Inc. All Rights Reserved.


#include "Framework/MagicTourPlayerController.h"
#include "Framework/MagicTourGameMode.h"
#include "Components/HealthComponent.h"
#include "Components/Button.h"
#include "Characters/MagicTourCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
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
	OnPossessedPawnChanged.AddUniqueDynamic(this, &AMagicTourPlayerController::PawnChanged);
	Progress = GetWorld() ? GetWorld()->GetAuthGameMode<AMagicTourGameMode>() : nullptr;
	if (Progress)
	{
		Progress->OnVictory.AddUniqueDynamic(this, &AMagicTourPlayerController::VictoryReady);
		isVictoryPending = Progress->IsVictory() && Progress->GetStage() != EMagicTourStage::Complete;
	}

	if (!IsLocalPlayerController())
	{
		PawnChanged(nullptr, GetPawn());
		return;
	}

	// Create the persistent gameplay HUD before optional touch controls.
	HUDWidget = CreateWidget<UMagicTourHUDWidget>(this, UMagicTourHUDWidget::StaticClass());
	if (HUDWidget)
	{
		HUDWidget->SetProgress(Progress);
		HUDWidget->AddToPlayerScreen(10);
		HUDWidget->OnRespawn.AddUniqueDynamic(this, &AMagicTourPlayerController::RespawnClicked);
		HUDWidget->OnConfirm.AddUniqueDynamic(this, &AMagicTourPlayerController::VictoryClicked);
	}
	else
	{
		UE_LOG(LogMagicTour, Error, TEXT("Could not spawn the gameplay HUD widget."));
	}

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
		Health->OnDied.RemoveDynamic(this, &AMagicTourPlayerController::PlayerDied);
	}

	Health = NewPawn ? NewPawn->FindComponentByClass<UHealthComponent>() : nullptr;
	if (Health)
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &AMagicTourPlayerController::HealthChanged);
		Health->OnDied.AddUniqueDynamic(this, &AMagicTourPlayerController::PlayerDied);
	}
	// Capture the first living pawn once; later respawns must not move the starting point.
	if (!isSpawnSet && NewPawn && Health && !Health->IsDead())
	{
		Spawn = NewPawn->GetActorTransform();
		isSpawnSet = true;
	}

	// Synchronize immediately; possession can happen after component BeginPlay.
	HealthChanged(Health ? Health->GetHealth() : 0.0f, Health ? Health->GetMaxHealth() : 0.0f);
	SetDead(Health && Health->IsDead());
}

void AMagicTourPlayerController::PlayerDied()
{
	SetDead(true);
}

void AMagicTourPlayerController::SetDead(bool Value)
{
	isDead = Value;
	UpdateUI();
}

void AMagicTourPlayerController::VictoryReady()
{
	if (Progress && Progress->GetStage() != EMagicTourStage::Complete)
	{
		isVictoryPending = true;
		UpdateUI();
	}
}

void AMagicTourPlayerController::VictoryClicked()
{
	ConfirmVictory();
}

bool AMagicTourPlayerController::ConfirmVictory()
{
	// Death takes priority even when the last opponent dies in the same frame.
	if (!isVictoryPending || isDead || !Progress || !Progress->IsVictory()) { return false; }
	Progress->Complete();
	isVictoryPending = false;
	UpdateUI();
	return true;
}

void AMagicTourPlayerController::UpdateUI()
{
	if (HUDWidget)
	{
		HUDWidget->ShowDeath(isDead);
		HUDWidget->ShowVictory(isVictoryPending);
	}
	if (!IsLocalPlayerController()) { return; }
	const bool isModal = isDead || isVictoryPending;
	bShowMouseCursor = isModal;
	if (MobileControlsWidget)
	{
		MobileControlsWidget->SetVisibility(isModal ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (isModal)
	{
		FInputModeUIOnly Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		UButton* Button = HUDWidget ? (isDead ? HUDWidget->GetRespawnButton() : HUDWidget->GetConfirmButton()) : nullptr;
		if (Button)
		{
			Mode.SetWidgetToFocus(Button->TakeWidget());
		}
		SetInputMode(Mode);
	}
	else
	{
		// Consume the menu click so the same press cannot also cast a spell afterward.
		SetInputMode(FInputModeGameOnly());
		FlushPressedKeys();
	}
}

void AMagicTourPlayerController::RespawnClicked()
{
	Respawn();
}

bool AMagicTourPlayerController::Respawn()
{
	APawn* Body = GetPawn();
	if (isRespawning || !isSpawnSet || !Body || !Health || !Health->IsDead() || !GetWorld())
	{
		return false;
	}
	isRespawning = true;
	// Construct a fresh character to restore health, animation, collision, and flight defaults.
	// Keep the corpse and its dialog intact if the starting point cannot accept a new pawn.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	// The stored transform already includes the Blueprint's root scale.
	Params.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;
	APawn* Pawn = GetWorld()->SpawnActor<APawn>(Body->GetClass(), Spawn, Params);
	if (!Pawn)
	{
		isRespawning = false;
		UE_LOG(LogMagicTour, Warning, TEXT("Could not respawn the player at the starting point."));
		return false;
	}
	Possess(Pawn);
	if (GetPawn() != Pawn)
	{
		Pawn->Destroy();
		Possess(Body);
		isRespawning = false;
		return false;
	}
	SetControlRotation(Spawn.Rotator());
	ClientSetRotation(Spawn.Rotator(), true);
	Body->Destroy();
	isRespawning = false;
	return true;
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
	if (HUDWidget)
	{
		HUDWidget->OnRespawn.RemoveDynamic(this, &AMagicTourPlayerController::RespawnClicked);
		HUDWidget->OnConfirm.RemoveDynamic(this, &AMagicTourPlayerController::VictoryClicked);
	}
	if (Progress) { Progress->OnVictory.RemoveDynamic(this, &AMagicTourPlayerController::VictoryReady); }
	isVictoryPending = false;
	PawnChanged(GetPawn(), nullptr);
	Super::EndPlay(Reason);
}

void AMagicTourPlayerController::CastInput()
{
	if (isDead || isRespawning || isVictoryPending) { return; }
	if (AMagicTourCharacter* Character = Cast<AMagicTourCharacter>(GetPawn()))
	{
		Character->CastSpell();
	}
}
