// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MagicTourCharacter.generated.h"

class UInputComponent;
class UInputAction;
class USceneComponent;
class UStaticMeshComponent;
class UFlightComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Base character for player-controlled ground movement.
 */
UCLASS(abstract)
class AMagicTourCharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Toggle Flight Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleFlightAction;

	/** Ascend Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AscendAction;

	/** Descend Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DescendAction;

	/** Boost Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BoostAction;

	/** Flight behavior shared by C++ and Blueprint presentation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flight", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UFlightComponent> Flight;

	/** Blueprint-adjustable attachment point for the placeholder broom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flight|Broom", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> BroomRoot;

	/** Replaceable placeholder broom mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flight|Broom", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> BroomMesh;

	/** Attachment point reserved for a future flight trail effect. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flight|Broom", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> BroomTrailPoint;
	
public:
	AMagicTourCharacter();

protected:
	virtual void BeginPlay() override;

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Toggles flight mode. */
	void ToggleFlight();

	/** Adds upward flight input. */
	void AscendInput();

	/** Adds downward flight input. */
	void DescendInput();

	/** Enables flight boost. */
	void BoostStart();

	/** Disables flight boost. */
	void BoostEnd();

	/** Updates presentation components when flight state changes. */
	UFUNCTION()
	void FlightChanged(bool isFlying);

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
};
