#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthChanged, float, Current, float, Max);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHealthDied);

/** Shared health storage; damage routing belongs to the owning actor. */
UCLASS(ClassGroup=(MagicTour), meta=(BlueprintSpawnableComponent))
class MAGICTOUR_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	/** Apply positive finite damage and return the actual health lost. */
	UFUNCTION(BlueprintCallable, Category="Health")
	float ApplyDamage(float Amount);

	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealth() const { return Current; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return Max; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetRatio() const { return Max > 0.0f ? Current / Max : 0.0f; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return Current <= 0.0f; }

	UPROPERTY(BlueprintAssignable, Category="Health")
	FHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FHealthDied OnDied;

protected:
	virtual void BeginPlay() override;

	/** Configure starting capacity in Blueprint defaults, never during damage dispatch. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(ClampMin="1.0"))
	float Max = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float Current = 100.0f;
};
