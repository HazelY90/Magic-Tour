// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlightComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFlightChanged, bool, isFlying);

/**
 * Coordinates arcade-style flight using the owning character's movement component.
 */
UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class MAGICTOUR_API UFlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlightComponent();

	/** Toggles between flight and gravity-driven ground movement. */
	UFUNCTION(BlueprintCallable, Category="Flight")
	void ToggleFlight();

	/** Adds vertical flight input along the world Z axis. */
	UFUNCTION(BlueprintCallable, Category="Flight")
	void MoveVertical(float Value);

	/** Enables the configured boost speed while flying. */
	UFUNCTION(BlueprintCallable, Category="Flight")
	void StartBoost();

	/** Restores the normal flight speed. */
	UFUNCTION(BlueprintCallable, Category="Flight")
	void StopBoost();

	/** Returns true while the movement component is using MOVE_Flying. */
	UFUNCTION(BlueprintPure, Category="Flight")
	bool IsFlying() const;

	/** Broadcast when the character enters or leaves flight. */
	UPROPERTY(BlueprintAssignable, Category="Flight")
	FOnFlightChanged OnFlightChanged;

protected:
	virtual void BeginPlay() override;

	/** Maximum speed during normal flight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Movement", meta=(ClampMin="0.0"))
	float FlySpeed = 900.0f;

	/** Maximum speed while boost is held. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Movement", meta=(ClampMin="0.0"))
	float BoostSpeed = 1600.0f;

	/** Acceleration used while flying. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Movement", meta=(ClampMin="0.0"))
	float Accel = 2400.0f;

	/** Braking applied when flight input is released. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Movement", meta=(ClampMin="0.0"))
	float Brake = 1800.0f;

private:
	ACharacter* GetCharacter() const;
	UCharacterMovementComponent* GetMove() const;
	void SetFlight(bool isFlying);
	void ApplySpeed(bool isBoosting);
	float GroundAccel = 0.0f;
};
