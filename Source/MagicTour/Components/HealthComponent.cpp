#include "Components/HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Editor metadata cannot protect against invalid serialized values.
	Max = FMath::IsFinite(Max) ? FMath::Max(1.0f, Max) : 100.0f;
	Current = Max;
	OnHealthChanged.Broadcast(Current, Max);
}

float UHealthComponent::ApplyDamage(float Amount)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	const float Lost = FMath::Min(Amount, Current);
	Current -= Lost;
	const bool isKilled = IsDead();

	// Commit state before callbacks so reentrant damage cannot repeat death.
	OnHealthChanged.Broadcast(Current, Max);
	if (isKilled)
	{
		OnDied.Broadcast();
	}
	return Lost;
}
