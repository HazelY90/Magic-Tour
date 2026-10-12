#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AEnemyCharacter;

/** Bounded patrol and stationary ranged attacks, without a behavior tree. */
UCLASS()
class MAGICTOUR_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();
	virtual void Tick(float Delta) override;
	void Stop();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="100.0"))
	float Range = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="0.4"))
	float Interval = 1.5f;

private:
	void GoTo(AEnemyCharacter* Enemy, const FVector& Pos);
	void Patrol(AEnemyCharacter* Enemy);

	double NextAttack = 0.0;
	double NextPatrol = 0.0;
	FVector Goal = FVector::ZeroVector;
};
