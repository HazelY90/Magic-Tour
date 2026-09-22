// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/MagicTourGameMode.h"
#include "GameFramework/Actor.h"
#include "Checkpoint.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

/** Ordered route trigger with a Blueprint-adjustable visible marker. */
UCLASS()
class MAGICTOUR_API ACheckpoint : public AActor
{
	GENERATED_BODY()

public:
	ACheckpoint();

	/** Returns whether this checkpoint has advanced the route. */
	UFUNCTION(BlueprintPure, Category="Checkpoint")
	bool IsReached() const { return isComplete; }

protected:
	/** One-based route position used for strict ordering. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Checkpoint", meta=(ClampMin="1"))
	int32 Index = 1;

	/** Task stage activated after this checkpoint is reached. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Checkpoint")
	EMagicTourStage NextStage = EMagicTourStage::ExploreForest;

	/** Trigger volume sized for both walking and low-altitude flight. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Checkpoint")
	TObjectPtr<UBoxComponent> Trigger;

	/** Hidden placeholder retained for optional future checkpoint presentation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Checkpoint")
	TObjectPtr<UStaticMeshComponent> Marker;

private:
	UFUNCTION()
	void Overlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool isFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleInstanceOnly, Category="Checkpoint")
	bool isComplete = false;
};
