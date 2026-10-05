#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Controller.h"
#include "SpellTestController.generated.h"

/** Deterministic camera view for isolated third-person aiming tests. */
UCLASS(Transient, NotBlueprintable)
class ASpellTestController : public AController
{
	GENERATED_BODY()
public:
	FVector View = FVector(-300.0f, 0.0f, 150.0f);
	FRotator Rot = FRotator::ZeroRotator;
	virtual void GetPlayerViewPoint(FVector& Pos, FRotator& Rotation) const override
	{
		Pos = View;
		Rotation = Rot;
	}
};
