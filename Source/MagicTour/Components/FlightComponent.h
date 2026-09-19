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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Toggles between flight and gravity-driven ground movement. */
	UFUNCTION(BlueprintCallable, Category="Flight")
	void ToggleFlight();

	/** Adds vertical flight input along the world Z axis. */
	UFUNCTION(BlueprintCallable, Category="Flight")
	void MoveVertical(float Value);

	/** Returns true while the movement component is using MOVE_Flying. */
	UFUNCTION(BlueprintPure, Category="Flight")
	bool IsFlying() const;

	/** Returns true while the automatic takeoff lift is active. */
	UFUNCTION(BlueprintPure, Category="Flight")
	bool IsTakingOff() const { return isTakingOff; }

	/** Broadcast when the character enters or leaves flight. */
	UPROPERTY(BlueprintAssignable, Category="Flight")
	FOnFlightChanged OnFlightChanged;

protected:
	virtual void BeginPlay() override;

	/** Height added to the character's position when automatic takeoff begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Takeoff", meta=(ClampMin="0.0"))
	float TakeoffHeight = 200.0f;

	/** Maximum upward speed during automatic takeoff. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Takeoff", meta=(ClampMin="0.0"))
	float TakeoffSpeed = 350.0f;

	/** Deceleration used while approaching the automatic takeoff height. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Takeoff", meta=(ClampMin="0.0"))
	float TakeoffBrake = 900.0f;

	/** Maximum time allowed for automatic takeoff before normal flight begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight|Takeoff", meta=(ClampMin="0.0"))
	float TakeoffDuration = 2.0f;

private:
	ACharacter* GetCharacter() const;
	UCharacterMovementComponent* GetMove() const;
	void SetFlight(bool isFlying);
	void BeginTakeoff();
	void EndTakeoff();
	float TakeoffZ = 0.0f;
	float TakeoffTime = 0.0f;
	float LastTakeoffZ = 0.0f;
	float StallTime = 0.0f;
	float FlightFloorZ = 0.0f;
	bool isTakingOff = false;
	bool isFlightFloorSet = false;
};
