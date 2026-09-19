// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/FlightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UFlightComponent::UFlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UFlightComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Move = GetMove())
	{
		// Keep walking and flying movement parameters in sync.
		Move->MaxFlySpeed = Move->MaxWalkSpeed;
		Move->BrakingDecelerationFlying = Move->BrakingDecelerationWalking;
		Move->AddTickPrerequisiteComponent(this);
	}
}

void UFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = GetCharacter();
	UCharacterMovementComponent* Move = GetMove();
	if (!Character || !Move || !IsFlying())
	{
		isTakingOff = false;
		isFlightFloorSet = false;
		SetComponentTickEnabled(false);
		return;
	}

	if (!isTakingOff)
	{
		if (isFlightFloorSet)
		{
			FVector Location = Character->GetActorLocation();
			if (Location.Z < FlightFloorZ)
			{
				Location.Z = FlightFloorZ;
				Character->SetActorLocation(Location, true);
			}

			if (Character->GetActorLocation().Z <= FlightFloorZ + 1.0f && Move->Velocity.Z < 0.0f)
			{
				Move->Velocity.Z = 0.0f;
			}
		}
		return;
	}

	TakeoffTime += DeltaTime;
	const float CurrentZ = Character->GetActorLocation().Z;
	const float Remaining = TakeoffZ - CurrentZ;
	if (Remaining <= 2.0f || (TakeoffDuration > 0.0f && TakeoffTime >= TakeoffDuration))
	{
		EndTakeoff();
		return;
	}

	StallTime = CurrentZ <= LastTakeoffZ + 0.1f ? StallTime + DeltaTime : 0.0f;
	LastTakeoffZ = CurrentZ;
	if (StallTime >= 0.2f)
	{
		EndTakeoff();
		return;
	}

	const float BrakeSpeed = TakeoffBrake > UE_KINDA_SMALL_NUMBER
		? FMath::Sqrt(2.0f * TakeoffBrake * Remaining)
		: TakeoffSpeed;
	Move->Velocity.Z = FMath::Min(TakeoffSpeed, BrakeSpeed);
}

void UFlightComponent::ToggleFlight()
{
	SetFlight(!IsFlying());
}

void UFlightComponent::MoveVertical(float Value)
{
	if (ACharacter* Character = GetCharacter(); Character && IsFlying())
	{
		if (Value < 0.0f && (isTakingOff || (isFlightFloorSet && Character->GetActorLocation().Z <= FlightFloorZ + 1.0f)))
		{
			if (UCharacterMovementComponent* Move = GetMove(); Move && Move->Velocity.Z < 0.0f)
			{
				Move->Velocity.Z = 0.0f;
			}
			return;
		}

		Character->AddMovementInput(FVector::UpVector, Value);
	}
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
		// Re-sync in case the shared ground settings changed at runtime.
		Move->MaxFlySpeed = Move->MaxWalkSpeed;
		Move->BrakingDecelerationFlying = Move->BrakingDecelerationWalking;
		Move->SetMovementMode(MOVE_Flying);
		BeginTakeoff();
	}
	else
	{
		EndTakeoff();
		isFlightFloorSet = false;
		SetComponentTickEnabled(false);
		Move->Velocity = FVector::ZeroVector;
		Move->SetMovementMode(MOVE_Falling);
	}

	OnFlightChanged.Broadcast(isFlying);
}

void UFlightComponent::BeginTakeoff()
{
	ACharacter* Character = GetCharacter();
	isFlightFloorSet = false;
	if (!Character)
	{
		SetComponentTickEnabled(false);
		return;
	}
	if (TakeoffHeight <= 0.0f || TakeoffSpeed <= 0.0f)
	{
		FlightFloorZ = Character->GetActorLocation().Z;
		isFlightFloorSet = true;
		isTakingOff = false;
		SetComponentTickEnabled(true);
		return;
	}

	TakeoffZ = Character->GetActorLocation().Z + TakeoffHeight;
	LastTakeoffZ = Character->GetActorLocation().Z;
	TakeoffTime = 0.0f;
	StallTime = 0.0f;
	isTakingOff = true;
	SetComponentTickEnabled(true);
}

void UFlightComponent::EndTakeoff()
{
	if (isTakingOff)
	{
		if (UCharacterMovementComponent* Move = GetMove(); Move && Move->Velocity.Z > 0.0f)
		{
			Move->Velocity.Z = 0.0f;
		}
		if (ACharacter* Character = GetCharacter(); Character && IsFlying())
		{
			FlightFloorZ = Character->GetActorLocation().Z;
			isFlightFloorSet = true;
		}
	}

	isTakingOff = false;
	SetComponentTickEnabled(IsFlying() && isFlightFloorSet);
}
