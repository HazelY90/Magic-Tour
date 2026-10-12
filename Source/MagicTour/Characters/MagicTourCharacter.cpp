// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/MagicTourCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/FlightComponent.h"
#include "Components/HealthComponent.h"
#include "Combat/SpellProjectile.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "MagicTour.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

AMagicTourCharacter::AMagicTourCharacter()
{
	// Gameplay notifies and the hand muzzle must update even when the mesh is not rendered.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Configure the collision capsule for the third-person character.
	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Keep the camera independent while movement rotates the character.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	Move->BrakingDecelerationFalling = 1500.0f;
	Move->AirControl = 0.5f;

	Flight = CreateDefaultSubobject<UFlightComponent>(TEXT("Flight"));
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	SpellClass = ASpellProjectile::StaticClass();
	CastMontage = FSoftObjectPath(TEXT("/Game/MagicTour/Characters/Animations/AM_Cast.AM_Cast"));
	CastPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CastPoint"));
	CastPoint->SetupAttachment(GetMesh(), TEXT("hand_r"));
	CastPoint->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));

	// Attach the placeholder broom to the character skeleton and expose its transform to Blueprint.
	BroomRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BroomRoot"));
	BroomRoot->SetupAttachment(GetMesh(), TEXT("pelvis"));
	BroomRoot->SetRelativeLocation(FVector(-25.0f, 0.0f, -15.0f));
	BroomRoot->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BroomRoot->SetVisibility(false, true);

	BroomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroomMesh"));
	BroomMesh->SetupAttachment(BroomRoot);
	BroomMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BroomMesh->SetRelativeScale3D(FVector(0.025f, 0.025f, 1.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BroomAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (BroomAsset.Succeeded())
	{
		BroomMesh->SetStaticMesh(BroomAsset.Object);
	}

	BroomTrailPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BroomTrailPoint"));
	BroomTrailPoint->SetupAttachment(BroomRoot);
	BroomTrailPoint->SetRelativeLocation(FVector(0.0f, 0.0f, -60.0f));
}

void AMagicTourCharacter::BeginPlay()
{
	Super::BeginPlay();

	Health->OnDied.AddDynamic(this, &AMagicTourCharacter::CancelCast);
	OnCharacterMovementUpdated.AddDynamic(this, &AMagicTourCharacter::MoveUpdated);

	if (Flight)
	{
		Flight->OnFlightChanged.AddDynamic(this, &AMagicTourCharacter::FlightChanged);
		FlightChanged(Flight->IsFlying());
	}
}

void AMagicTourCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMagicTourCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMagicTourCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::LookInput);

		// Flying
		EnhancedInputComponent->BindAction(ToggleFlightAction, ETriggerEvent::Started, this, &AMagicTourCharacter::ToggleFlight);
		EnhancedInputComponent->BindAction(AscendAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::AscendInput);
		EnhancedInputComponent->BindAction(DescendAction, ETriggerEvent::Triggered, this, &AMagicTourCharacter::DescendInput);
	}
	else
	{
		UE_LOG(LogMagicTour, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AMagicTourCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AMagicTourCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AMagicTourCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMagicTourCharacter::DoMove(float Right, float Forward)
{
	if (Controller)
	{
		// Cancel before movement or animation ticks can release the pending projectile.
		if (isCasting && !isReleased && !FVector2D(Right, Forward).IsNearlyZero())
		{
			CancelCast();
		}

		// Use camera yaw so ground movement follows the player's view.
		const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Rgt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

		AddMovementInput(Rgt, Right);
		AddMovementInput(Fwd, Forward);
	}
}

void AMagicTourCharacter::DoJumpStart()
{
	if (Flight && Flight->IsFlying())
	{
		return;
	}

	// pass Jump to the character
	Jump();
}

void AMagicTourCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AMagicTourCharacter::ToggleFlight()
{
	if (Flight)
	{
		Flight->ToggleFlight();
	}
}

void AMagicTourCharacter::AscendInput()
{
	if (Flight)
	{
		Flight->MoveVertical(1.0f);
	}
}

void AMagicTourCharacter::DescendInput()
{
	if (Flight)
	{
		Flight->MoveVertical(-1.0f);
	}
}

void AMagicTourCharacter::FlightChanged(bool isFlying)
{
	if (BroomRoot)
	{
		BroomRoot->SetVisibility(isFlying, true);
	}
}

float AMagicTourCharacter::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
	if (!Health || Health->IsDead() || !FMath::IsFinite(Amount) || Amount <= 0.0f || !CanBeDamaged())
	{
		return 0.0f;
	}

	const float Accepted = Super::TakeDamage(Amount, Event, Instigator, Causer);
	return Health->ApplyDamage(Accepted);
}

bool AMagicTourCharacter::IsCastAllowed() const
{
	return IsCastStateValid() && !isCasting && GetWorld()->GetTimeSeconds() >= NextCast;
}

bool AMagicTourCharacter::GetAim(FVector& Pos, FVector& Dir) const
{
	if (!Controller || !CastPoint || !GetWorld())
	{
		return false;
	}

	FVector View;
	FRotator Rot;
	Controller->GetPlayerViewPoint(View, Rot);
	const FVector Forward = Rot.Vector();
	const float Range = FMath::IsFinite(AimRange) ? FMath::Max(100.0f, AimRange) : 10000.0f;
	FVector Target = View + Forward * Range;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(SpellAim), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, View, Target, ECC_Visibility, Query))
	{
		Target = Hit.ImpactPoint;
	}

	Pos = CastPoint->GetComponentLocation();
	// Preserve convergence for normal targets, but cap near-hit deflection at 45 degrees.
	const FVector AimDir = (Target - Pos).GetSafeNormal();
	if (FVector::DotProduct(AimDir, Forward) < 0.70710678f)
	{
		Target = Pos + Forward * Range;
	}
	Dir = (Target - Pos).GetSafeNormal();
	return !Pos.ContainsNaN() && !Dir.ContainsNaN() && !Dir.IsNearlyZero();
}

bool AMagicTourCharacter::SpawnSpell()
{
	if (!IsCastStateValid() || !SpellClass)
	{
		return false;
	}
	FVector Pos, Dir;
	if (!GetAim(Pos, Dir))
	{
		return false;
	}

	const ASpellProjectile* Defaults = SpellClass.GetDefaultObject();
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Defaults->GetRadius());
	FCollisionQueryParams Query(SCENE_QUERY_STAT(SpellSpawn), false, this);
	FCollisionResponseParams Responses;
	Responses.CollisionResponse.SetAllChannels(ECR_Ignore);
	Responses.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	FHitResult Hit;

	// Prevent an offset muzzle from appearing across a wall or inside another actor.
	if (GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), Pos, FQuat::Identity,
		ECC_WorldDynamic, Shape, Query, Responses)
		|| GetWorld()->OverlapBlockingTestByChannel(Pos, FQuat::Identity,
			ECC_WorldDynamic, Shape, Query, Responses))
	{
		return false;
	}

	const FTransform Transform(Dir.Rotation(), Pos);
	ASpellProjectile* Spell = GetWorld()->SpawnActorDeferred<ASpellProjectile>(SpellClass,
		Transform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Spell)
	{
		return false;
	}
	Spell->Init(this, Dir);
	UGameplayStatics::FinishSpawningActor(Spell, Transform);
	return true;
}

bool AMagicTourCharacter::IsCastStateValid() const
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	// Check queued input as well as motion to reject casting in the input's first frame.
	return Health && !Health->IsDead() && Controller && GetWorld()
		&& Move->MovementMode == MOVE_Walking && Move->Velocity.IsNearlyZero(1.0f)
		&& Move->GetCurrentAcceleration().IsNearlyZero() && GetPendingMovementInputVector().IsNearlyZero();
}

bool AMagicTourCharacter::CastSpell()
{
	if (!IsCastAllowed() || !SpellClass)
	{
		return false;
	}
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	UAnimMontage* Montage = CastMontage.LoadSynchronous();
	if (!Anim || !Montage)
	{
		return false;
	}
	++CastId;
	ActiveCast = Montage;
	isCasting = true;
	isReleased = false;
	if (Anim->Montage_Play(Montage) <= 0.0f)
	{
		isCasting = false;
		ActiveCast = nullptr;
		return false;
	}
	CastInstance = Anim->GetActiveInstanceForMontage(Montage)->GetInstanceID();
	FOnMontageEnded End;
	End.BindUObject(this, &AMagicTourCharacter::CastEnded, CastId);
	Anim->Montage_SetEndDelegate(End, Montage);

	// Face the aim while keeping the camera independent and movement available to cancel.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	isMoveOriented = Move->bOrientRotationToMovement;
	isControllerOriented = Move->bUseControllerDesiredRotation;
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = true;
	SetActorRotation(FRotator(0.0f, Controller->GetControlRotation().Yaw, 0.0f));
	const float Delay = FMath::IsFinite(Cooldown) ? FMath::Max(0.0f, Cooldown) : 0.4f;
	NextCast = GetWorld()->GetTimeSeconds() + Delay;
	return true;
}

bool AMagicTourCharacter::ReleaseSpell(UAnimSequenceBase* Animation, int32 InstanceId)
{
	if (!isCasting || isReleased || Animation != ActiveCast
		|| (InstanceId != INDEX_NONE && InstanceId != CastInstance))
	{
		return false;
	}
	if (!IsCastStateValid())
	{
		CancelCast();
		return false;
	}
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	if (!Anim || !Anim->Montage_IsPlaying(ActiveCast))
	{
		return false;
	}
	// Consume the notification before spawning to guard against duplicate callbacks.
	isReleased = true;
	return SpawnSpell();
}

void AMagicTourCharacter::CancelCast()
{
	if (!isCasting)
	{
		return;
	}
	UAnimMontage* Montage = ActiveCast;
	isCasting = false;
	isReleased = false;
	ActiveCast = nullptr;
	CastInstance = INDEX_NONE;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = isMoveOriented;
	Move->bUseControllerDesiredRotation = isControllerOriented;
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		Anim->Montage_Stop(0.1f, Montage);
	}
}

void AMagicTourCharacter::CastEnded(UAnimMontage* Montage, bool isInterrupted, uint32 Id)
{
	// An old instance may finish blending after the same montage has been restarted.
	if (Id == CastId && Montage == ActiveCast)
	{
		CancelCast();
	}
}

void AMagicTourCharacter::MoveUpdated(float Delta, FVector OldPos, FVector OldVel)
{
	// Detect external movement too, including a displacement that ends with zero velocity.
	if (isCasting && !isReleased
		&& (!IsCastStateValid() || !GetActorLocation().Equals(OldPos, 0.01f)))
	{
		CancelCast();
	}
}

void AMagicTourCharacter::OnMovementModeChanged(EMovementMode PrevMode, uint8 PrevCustomMode)
{
	Super::OnMovementModeChanged(PrevMode, PrevCustomMode);
	if (GetCharacterMovement()->MovementMode != MOVE_Walking)
	{
		CancelCast();
	}
}

void AMagicTourCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	CancelCast();
	OnCharacterMovementUpdated.RemoveDynamic(this, &AMagicTourCharacter::MoveUpdated);
	if (Health)
	{
		Health->OnDied.RemoveDynamic(this, &AMagicTourCharacter::CancelCast);
	}
	Super::EndPlay(Reason);
}
