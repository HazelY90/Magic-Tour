// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/FlightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UFlightComponent::UFlightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlightComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Move = GetMove())
	{
		GroundAccel = Move->MaxAcceleration;
		Move->MaxFlySpeed = FlySpeed;
		Move->BrakingDecelerationFlying = Brake;
	}
}

void UFlightComponent::ToggleFlight()
{
	SetFlight(!IsFlying());
}

void UFlightComponent::MoveVertical(float Value)
{
	if (ACharacter* Character = GetCharacter(); Character && IsFlying())
	{
		Character->AddMovementInput(FVector::UpVector, Value);
	}
}

void UFlightComponent::StartBoost()
{
	if (IsFlying())
	{
		ApplySpeed(true);
	}
}

void UFlightComponent::StopBoost()
{
	ApplySpeed(false);
}

bool UFlightComponent::IsFlying() const
{
	const UCharacterMovementComponent* Move = GetMove();
	return Move && Move->MovementMode == MOVE_Flying;
}

ACharacter* UFlightComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

UCharacterMovementComponent* UFlightComponent::GetMove() const
{
	const ACharacter* Character = GetCharacter();
	return Character ? Character->GetCharacterMovement() : nullptr;
}

void UFlightComponent::SetFlight(bool isFlying)
{
	UCharacterMovementComponent* Move = GetMove();
	if (!Move || isFlying == IsFlying())
	{
		return;
	}

	if (isFlying)
	{
		Move->Velocity.Z = 0.0f;
		Move->MaxAcceleration = Accel;
		Move->BrakingDecelerationFlying = Brake;
		ApplySpeed(false);
		Move->SetMovementMode(MOVE_Flying);
	}
	else
	{
		ApplySpeed(false);
		Move->MaxAcceleration = GroundAccel;
		Move->SetMovementMode(MOVE_Falling);
	}

	OnFlightChanged.Broadcast(isFlying);
}

void UFlightComponent::ApplySpeed(bool isBoosting)
{
	if (UCharacterMovementComponent* Move = GetMove())
	{
		Move->MaxFlySpeed = isBoosting ? BoostSpeed : FlySpeed;
	}
}
