#pragma once

#include "CoreMinimal.h"
#include "Characters/MagicTourCharacter.h"
#include "EnemyCharacter.generated.h"

class ACombatArena;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Patrol,
	Chase,
	Attack,
	Dead
};

/** Ruins opponent sharing the player's stationary casting and health lifecycle. */
UCLASS()
class MAGICTOUR_API AEnemyCharacter : public AMagicTourCharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();
	virtual bool GetAim(FVector& Pos, FVector& Dir) const override;
	virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;

	void SetTarget(AMagicTourCharacter* Player);
	AMagicTourCharacter* GetTarget() const { return Target; }
	bool IsTargetValid() const;
	EEnemyState GetState() const { return State; }
	void SetState(EEnemyState Value) { if (State != EEnemyState::Dead) { State = Value; } }

	/** Assign the single encounter region to each of the three placed enemies. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Enemy")
	TObjectPtr<ACombatArena> Arena;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION()
	void Die();

	UPROPERTY(Transient, VisibleInstanceOnly, Category="Enemy")
	TObjectPtr<AMagicTourCharacter> Target;

	UPROPERTY(VisibleInstanceOnly, Category="Enemy")
	EEnemyState State = EEnemyState::Patrol;
};
