#include "AI/EnemyAIController.h"

#include "Characters/EnemyCharacter.h"
#include "Components/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "World/CombatArena.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.15f;
}

void AEnemyAIController::Stop()
{
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	SetActorTickEnabled(false);
}

void AEnemyAIController::GoTo(AEnemyCharacter* Enemy, const FVector& Pos)
{
	if (!Enemy->Arena->IsInside(Pos)) { return; }
	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
		GetWorld(), Enemy->GetActorLocation(), Pos, this);
	if (!Path || !Path->IsValid() || Path->IsPartial() || !Enemy->Arena->IsPathInside(Path->PathPoints))
	{
		return;
	}
	// Use the validated path itself so the move cannot substitute an out-of-region route.
	FAIMoveRequest Request(Pos);
	Request.SetAcceptanceRadius(70.0f);
	Request.SetReachTestIncludesAgentRadius(true);
	Request.SetAllowPartialPath(false);
	const FAIRequestID Id = RequestMove(Request, Path->GetPath());
	if (!Id.IsValid()) { return; }
	Goal = Pos;
}

void AEnemyAIController::Patrol(AEnemyCharacter* Enemy)
{
	Enemy->SetState(EEnemyState::Patrol);
	if (GetMoveStatus() == EPathFollowingStatus::Moving || GetWorld()->GetTimeSeconds() < NextPatrol) { return; }
	NextPatrol = GetWorld()->GetTimeSeconds() + 1.0;
	FVector Pos;
	if (Enemy->Arena->IsPatrolFound(Enemy->GetActorLocation(), Pos)) { GoTo(Enemy, Pos); }
}

void AEnemyAIController::Tick(float Delta)
{
	Super::Tick(Delta);
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	if (!Enemy || Enemy->GetState() == EEnemyState::Dead || !IsValid(Enemy->Arena)) { return; }
	AMagicTourCharacter* Player = Cast<AMagicTourCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const bool isEngaged = Enemy->GetTarget() != nullptr;
	Enemy->SetTarget(Player);
	if (!Enemy->IsTargetValid())
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		if (isEngaged) { StopMovement(); }
		Patrol(Enemy);
		return;
	}
	SetFocus(Player);
	SetControlRotation((Player->GetActorLocation() - Enemy->GetActorLocation()).Rotation());
	if (Enemy->IsCasting()) { return; }
	const float Dist = FVector::Dist(Enemy->GetActorLocation(), Player->GetActorLocation());
	if (Dist <= Range && LineOfSightTo(Player))
	{
		Enemy->SetState(EEnemyState::Attack);
		StopMovement();
		// Both opponents and players obey the shared stationary casting gate.
		Enemy->GetCharacterMovement()->StopMovementImmediately();
		Enemy->ConsumeMovementInputVector();
		if (GetWorld()->GetTimeSeconds() >= NextAttack && Enemy->CastSpell())
		{
			NextAttack = GetWorld()->GetTimeSeconds() + Interval;
		}
	}
	else
	{
		Enemy->SetState(EEnemyState::Chase);
		if (GetMoveStatus() != EPathFollowingStatus::Moving || FVector::DistSquared2D(Goal, Player->GetActorLocation()) > FMath::Square(150.0f))
		{
			GoTo(Enemy, Player->GetActorLocation());
		}
	}
}
