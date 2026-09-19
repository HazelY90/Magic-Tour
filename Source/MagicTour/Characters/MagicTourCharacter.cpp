// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/MagicTourCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/FlightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "MagicTour.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

AMagicTourCharacter::AMagicTourCharacter()
{
	// Configure the collision capsule for the third-person character.
	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Keep the camera independent while movement rotates the character.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	Move->BrakingDecelerationFalling = 1500.0f;
	Move->AirControl = 0.5f;

	Flight = CreateDefaultSubobject<UFlightComponent>(TEXT("Flight"));

	// Attach the placeholder broom to the character skeleton and expose its transform to Blueprint.
	BroomRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BroomRoot"));
	BroomRoot->SetupAttachment(GetMesh(), TEXT("pelvis"));
	BroomRoot->SetRelativeLocation(FVector(-25.0f, 0.0f, -15.0f));
	BroomRoot->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BroomRoot->SetVisibility(false, true);

	BroomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroomMesh"));
	BroomMesh->SetupAttachment(BroomRoot);
	BroomMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BroomMesh->SetRelativeScale3D(FVector(0.025f, 0.025f, 1.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BroomAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (BroomAsset.Succeeded())
	{
		BroomMesh->SetStaticMesh(BroomAsset.Object);
	}

	BroomTrailPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BroomTrailPoint"));
	BroomTrailPoint->SetupAttachment(BroomRoot);
	BroomTrailPoint->SetRelativeLocation(FVector(0.0f, 0.0f, -60.0f));
}

void AMagicTourCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (Flight)
	{
		Flight->OnFlightChanged.AddDynamic(this, &AMagicTourCharacter::FlightChanged);
		FlightChanged(Flight->IsFlying());
	}
}

void AMagicTourCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMagicTourCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMagicTourCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::LookInput);

		// Flying
		EnhancedInputComponent->BindAction(ToggleFlightAction, ETriggerEvent::Started, this, &AMagicTourCharacter::ToggleFlight);
		EnhancedInputComponent->BindAction(AscendAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::AscendInput);
		EnhancedInputComponent->BindAction(DescendAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::DescendInput);
		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Started, this, &AMagicTourCharacter::BoostStart);
		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Completed, this, &AMagicTourCharacter::BoostEnd);
		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Canceled, this, &AMagicTourCharacter::BoostEnd);
	}
	else
	{
		UE_LOG(LogMagicTour, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AMagicTourCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AMagicTourCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AMagicTourCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMagicTourCharacter::DoMove(float Right, float Forward)
{
	if (Controller)
	{
		// Use camera yaw so ground movement follows the player's view.
		const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Rgt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

		AddMovementInput(Rgt, Right);
		AddMovementInput(Fwd, Forward);
	}
}

void AMagicTourCharacter::DoJumpStart()
{
	if (Flight && Flight->IsFlying())
	{
		return;
	}

	// pass Jump to the character
	Jump();
}

void AMagicTourCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AMagicTourCharacter::ToggleFlight()
{
	if (Flight)
	{
		Flight->ToggleFlight();
	}
}

void AMagicTourCharacter::AscendInput()
{
	if (Flight)
	{
		Flight->MoveVertical(1.0f);
	}
}

void AMagicTourCharacter::DescendInput()
{
	if (Flight)
	{
		Flight->MoveVertical(-1.0f);
	}
}

void AMagicTourCharacter::BoostStart()
{
	if (Flight)
	{
		Flight->StartBoost();
	}
}

void AMagicTourCharacter::BoostEnd()
{
	if (Flight)
	{
		Flight->StopBoost();
	}
}

void AMagicTourCharacter::FlightChanged(bool isFlying)
{
	if (BroomRoot)
	{
		BroomRoot->SetVisibility(isFlying, true);
	}
}
