// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/MagicTourGameMode.h"
#include "MagicTourHUDWidget.generated.h"

class AMagicTourGameMode;
class UProgressBar;
class UTextBlock;

/** Minimal gameplay HUD with reserved health and current task display. */
UCLASS()
class MAGICTOUR_API UMagicTourHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Connects the HUD to route progress and applies the initial task. */
	void SetProgress(AMagicTourGameMode* GameMode);

	/** Updates the reserved health bar when the health system becomes available. */
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
