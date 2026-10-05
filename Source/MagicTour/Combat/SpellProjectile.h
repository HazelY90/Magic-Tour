#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpellProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/** A swept, straight-flight spell with one damage settlement per impact. */
UCLASS()
class MAGICTOUR_API ASpellProjectile : public AActor
{
	GENERATED_BODY()
public:
	ASpellProjectile();

	/** Called before deferred spawning finishes so the caster is ignored immediately. */
	void Init(AActor* Caster, const FVector& Dir);
	float GetRadius() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell")
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell")
	TObjectPtr<UProjectileMovementComponent> Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell", meta=(ClampMin="0.0"))
	float Damage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell", meta=(ClampMin="1.0"))
	float Speed = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell", meta=(ClampMin="0.1"))
	float Life = 4.0f;

private:
	UFUNCTION()
	void Hit(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp,
		FVector Impulse, const FHitResult& Result);

	/** World-space direction supplied by aiming, independent of component initialization. */
	FVector LaunchDir = FVector::ZeroVector;
	bool isHit = false;
};
