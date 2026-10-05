#pragma once

#include "CoreMinimal.h"
#include "Components/HealthComponent.h"
#include "HealthTestObserver.generated.h"

/** Event probe for health automation tests, including reentrant damage. */
UCLASS(Transient, NotBlueprintable)
class UHealthTestObserver : public UObject
{
	GENERATED_BODY()
public:
	int32 Changes = 0;
	int32 Deaths = 0;
	bool isReentrant = false;
	UHealthComponent* Health = nullptr;

	UFUNCTION()
	void Changed(float Current, float Max)
	{
		++Changes;
		if (isReentrant && Health)
		{
			Health->ApplyDamage(1000.0f);
		}
	}

	UFUNCTION()
	void Died() { ++Deaths; }
};
