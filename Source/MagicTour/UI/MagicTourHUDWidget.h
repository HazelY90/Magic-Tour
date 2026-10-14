// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/MagicTourGameMode.h"
#include "MagicTourHUDWidget.generated.h"

class AMagicTourGameMode;
class UProgressBar;
class UTextBlock;
class UBorder;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRespawnRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVictoryConfirmed);

/** Minimal gameplay HUD with live health, task guidance, and a centered crosshair. */
UCLASS()
class MAGICTOUR_API UMagicTourHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Connects the HUD to route progress and applies the initial task. */
	void SetProgress(AMagicTourGameMode* GameMode);

	/** Updates the live health bar from the current pawn. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetHealth(float Value);

	/** Shows the death prompt while keeping route guidance connected. */
	void ShowDeath(bool isDead);

	/** Shows the victory prompt while allowing death to take priority. */
	void ShowVictory(bool isVisible);

	/** Exposes the action button for keyboard focus and integration checks. */
	UButton* GetRespawnButton() const { return RespawnButton; }
	UButton* GetConfirmButton() const { return ConfirmButton; }

	bool IsDeathShown() const { return isDeathShown; }
	bool IsVictoryShown() const { return isVictoryShown; }

	/** The controller owns respawning; the HUD only requests the action. */
	UPROPERTY(BlueprintAssignable, Category="HUD")
	FRespawnRequested OnRespawn;

	/** The controller completes the task after this confirmation. */
	UPROPERTY(BlueprintAssignable, Category="HUD")
	FVictoryConfirmed OnConfirm;

protected:
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void ProgressChanged(int32 ReachedIndex, int32 TotalCheckpoints, EMagicTourStage Stage);

	UFUNCTION()
	void RespawnClicked();

	UFUNCTION()
	void ConfirmClicked();

	void UpdateTask();
	void UpdateCrosshair();

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> TaskText;

	UPROPERTY()
	TObjectPtr<AMagicTourGameMode> Progress;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> Crosshair;

	UPROPERTY()
	TObjectPtr<UBorder> DeathOverlay;

	UPROPERTY()
	TObjectPtr<UButton> RespawnButton;

	UPROPERTY()
	TObjectPtr<UBorder> VictoryOverlay;

	UPROPERTY()
	TObjectPtr<UButton> ConfirmButton;

	bool isDeathShown = false;
	bool isVictoryShown = false;
};
