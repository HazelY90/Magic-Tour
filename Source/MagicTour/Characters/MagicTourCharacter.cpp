// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/MagicTourCharacter.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "MagicTour.h"
#include "Math/RotationMatrix.h"

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
	// pass Jump to the character
	Jump();
}

void AMagicTourCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
