#include "Characters/EnemyCharacter.h"

#include "AI/EnemyAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/CombatArena.h"
#include "UObject/ConstructorHelpers.h"

AEnemyCharacter::AEnemyCharacter()
{
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	GetCharacterMovement()->MaxWalkSpeed = 220.0f;
	// Navigation must drive acceleration so the shared locomotion graph recognizes walking.
	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = true;
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// Reuse the tested mannequin locomotion, cast slot, hand muzzle, and release notify.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(
		TEXT("/Game/MagicTour/Characters/Animations/ABP_MagicTour"));
	if (Mesh.Succeeded()) { GetMesh()->SetSkeletalMesh(Mesh.Object); }
	if (Anim.Succeeded()) { GetMesh()->SetAnimInstanceClass(Anim.Class); }
	Tags.Add(TEXT("Enemy"));
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health->OnDied.AddDynamic(this, &AEnemyCharacter::Die);
}

bool AEnemyCharacter::IsTargetValid() const
{
	const UHealthComponent* Life = IsValid(Target) ? Target->FindComponentByClass<UHealthComponent>() : nullptr;
	return Health && !Health->IsDead() && IsValid(Arena) && IsValid(Target)
		&& Target->IsPlayerControlled() && Life && !Life->IsDead()
		&& Arena->IsInside(Target->GetActorLocation());
}

void AEnemyCharacter::SetTarget(AMagicTourCharacter* Player)
{
	if (Target != Player)
	{
		// Losing or changing the target invalidates any pending animation release.
		CancelCast();
		Target = Player;
	}
	if (Target && !IsTargetValid())
	{
		CancelCast();
		Target = nullptr;
	}
}

bool AEnemyCharacter::GetAim(FVector& Pos, FVector& Dir) const
{
	if (!IsTargetValid() || !CastPoint) { return false; }
	Pos = CastPoint->GetComponentLocation();
	// Resolve the current player position at release rather than aiming through a camera.
	Dir = (Target->GetActorLocation() - Pos).GetSafeNormal();
	return !Pos.ContainsNaN() && !Dir.ContainsNaN() && !Dir.IsNearlyZero();
}

float AEnemyCharacter::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
	// Shared projectiles may hit another opponent, but opponents never damage each other.
	if ((Instigator && Cast<AEnemyCharacter>(Instigator->GetPawn()))
		|| Cast<AEnemyCharacter>(Causer) || (Causer && Cast<AEnemyCharacter>(Causer->GetOwner())))
	{
		return 0.0f;
	}
	return Super::TakeDamage(Amount, Event, Instigator, Causer);
}

void AEnemyCharacter::Die()
{
	State = EEnemyState::Dead;
	Target = nullptr;
	CancelCast();
	if (AEnemyAIController* AI = Cast<AEnemyAIController>(Controller)) { AI->Stop(); }
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Keep a short-lived frozen placeholder until a death animation is authored.
	GetMesh()->bPauseAnims = true;
	SetLifeSpan(5.0f);
}

void AEnemyCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	Health->OnDied.RemoveDynamic(this, &AEnemyCharacter::Die);
	Super::EndPlay(Reason);
}
