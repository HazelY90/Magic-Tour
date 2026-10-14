#include "Combat/SpellProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ASpellProjectile::ASpellProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(Sphere);
	Sphere->InitSphereRadius(8.0f);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Sphere->SetGenerateOverlapEvents(false);
	Sphere->OnComponentHit.AddDynamic(this, &ASpellProjectile::Hit);

	// Keep the placeholder visual separate from the swept collision root.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Sphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.16f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Asset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Asset.Succeeded())
	{
		Mesh->SetStaticMesh(Asset.Object);
	}

	Move = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Move"));
	Move->SetUpdatedComponent(Sphere);
	Move->bInitialVelocityInLocalSpace = false;
	Move->Velocity = FVector::ZeroVector;
	Move->InitialSpeed = Speed;
	Move->MaxSpeed = Speed;
	Move->ProjectileGravityScale = 0.0f;
	Move->bRotationFollowsVelocity = true;
	Move->bShouldBounce = false;
}

void ASpellProjectile::Init(AActor* Caster, const FVector& Dir, float Amount)
{
	Damage = FMath::IsFinite(Amount) ? FMath::Max(0.0f, Amount) : 0.0f;
	SetOwner(Caster);
	SetInstigator(Cast<APawn>(Caster));
	Sphere->IgnoreActorWhenMoving(Caster, true);
	LaunchDir = Dir.GetSafeNormal();
	Move->bInitialVelocityInLocalSpace = false;
	Move->Velocity = LaunchDir * Speed;
}

void ASpellProjectile::BeginPlay()
{
	Super::BeginPlay();
	// Apply Blueprint tuning after construction and sanitize serialized values.
	Speed = FMath::IsFinite(Speed) ? FMath::Max(1.0f, Speed) : 2500.0f;
	Life = FMath::IsFinite(Life) ? FMath::Max(0.1f, Life) : 4.0f;
	Damage = FMath::IsFinite(Damage) ? FMath::Max(0.0f, Damage) : 0.0f;
	Sphere->IgnoreActorWhenMoving(GetOwner(), true);
	Move->InitialSpeed = Speed;
	Move->MaxSpeed = Speed;
	// Never derive the launch direction from a rotation modified by movement initialization.
	const FVector Dir = LaunchDir.IsNearlyZero() ? GetActorForwardVector() : LaunchDir;
	Move->bInitialVelocityInLocalSpace = false;
	SetActorRotation(Dir.Rotation());
	Move->Velocity = Dir * Speed;
	Move->UpdateComponentVelocity();
	SetLifeSpan(Life);
}

float ASpellProjectile::GetRadius() const
{
	return Sphere->GetScaledSphereRadius();
}

void ASpellProjectile::Hit(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp,
	FVector Impulse, const FHitResult& Result)
{
	if (isHit || (Other && (Other == GetOwner() || Other == GetInstigator())))
	{
		return;
	}

	// Commit the impact before callbacks, then stop movement to prevent repeated hits.
	isHit = true;
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Move->StopMovementImmediately();
	if (Other)
	{
		UGameplayStatics::ApplyPointDamage(Other, Damage, GetActorForwardVector(), Result,
			GetInstigatorController(), this, nullptr);
	}
	Destroy();
}
