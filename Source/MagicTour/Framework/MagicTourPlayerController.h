// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagicTourPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UMagicTourHUDWidget;
class UHealthComponent;
class UInputAction;

/**
 *  Player Controller for the third-person character.
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class MAGICTOUR_API AMagicTourPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AMagicTourPlayerController();

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** Persistent gameplay HUD for health and task guidance. */
	UPROPERTY()
	TObjectPtr<UMagicTourHUDWidget> HUDWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Follow the current pawn and release delegate bindings during teardown. */
	UPROPERTY()
	TObjectPtr<UHealthComponent> Health;

	UFUNCTION()
	void PawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HealthChanged(float Current, float Max);

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Native default mapping keeps the first spell playable without an asset setup step. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CastAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> CastMap;

	void CastInput();

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
};
