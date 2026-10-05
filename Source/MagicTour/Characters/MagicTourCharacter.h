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
class UHealthComponent;
class ASpellProjectile;
class UAnimMontage;
class UAnimSequenceBase;
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

	/** Flight behavior shared by C++ and Blueprint presentation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flight", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UFlightComponent> Flight;

	/** Reusable health and damage events. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health")
	TObjectPtr<UHealthComponent> Health;

	/** Hand-attached muzzle with a Blueprint-adjustable local offset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell")
	TObjectPtr<USceneComponent> CastPoint;

	UPROPERTY(EditDefaultsOnly, Category="Spell")
	TSubclassOf<ASpellProjectile> SpellClass;

	UPROPERTY(EditDefaultsOnly, Category="Spell", meta=(ClampMin="100.0"))
	float AimRange = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category="Spell", meta=(ClampMin="0.0"))
	float Cooldown = 0.4f;

	/** Upper-body action asset; loaded only when casting is requested. */
	UPROPERTY(EditDefaultsOnly, Category="Spell")
	TSoftObjectPtr<UAnimMontage> CastMontage;

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

	/** Gate for starting new casting actions. */
	UFUNCTION(BlueprintPure, Category="Spell")
	bool IsCastAllowed() const;

	/** Resolve the center-screen aim point, including close-range fallback. */
	bool GetAim(FVector& Pos, FVector& Dir) const;

	/** Start the action; the montage notify releases the projectile later. */
	UFUNCTION(BlueprintCallable, Category="Spell")
	bool CastSpell();

	/** Accept only the active montage's release notification, once per action. */
	bool ReleaseSpell(UAnimSequenceBase* Animation, int32 InstanceId = INDEX_NONE);

	UFUNCTION(BlueprintPure, Category="Spell")
	bool IsCasting() const { return isCasting; }

	/** Route UE damage through the shared health component. */
	virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMode, uint8 PrevCustomMode = 0) override;

	UFUNCTION()
	void CancelCast();

	void CastEnded(UAnimMontage* Montage, bool isInterrupted, uint32 Id);
	bool IsCastStateValid() const;
	bool SpawnSpell();

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveCast;

	uint32 CastId = 0;
	int32 CastInstance = INDEX_NONE;
	bool isCasting = false;
	bool isReleased = false;
	bool isMoveOriented = true;
	bool isControllerOriented = false;

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

	/** Updates presentation components when flight state changes. */
	UFUNCTION()
	void FlightChanged(bool isFlying);

protected:

	/** World time of the next permitted successful cast. */
	double NextCast = 0.0;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
};
