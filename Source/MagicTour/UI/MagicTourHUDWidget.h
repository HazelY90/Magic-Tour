// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/MagicTourGameMode.h"
#include "MagicTourHUDWidget.generated.h"

class AMagicTourGameMode;
class UProgressBar;
class UTextBlock;

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

protected:
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void ProgressChanged(int32 ReachedIndex, int32 TotalCheckpoints, EMagicTourStage Stage);

	void UpdateTask();

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> TaskText;

	UPROPERTY()
	TObjectPtr<AMagicTourGameMode> Progress;
};
