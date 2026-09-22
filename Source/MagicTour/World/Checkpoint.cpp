// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Checkpoint.h"

#include "Characters/MagicTourCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACheckpoint::ACheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(450.0f, 450.0f, 500.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::Overlap);

	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Trigger);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetVisibility(false);
	Marker->SetHiddenInGame(true);
	Marker->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));
	Marker->SetRelativeScale3D(FVector(0.4f, 0.4f, 5.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MarkerAsset.Succeeded())
	{
		Marker->SetStaticMesh(MarkerAsset.Object);
	}
}

void ACheckpoint::Overlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool isFromSweep, const FHitResult& SweepResult)
{
	if (isComplete || !Cast<AMagicTourCharacter>(OtherActor))
	{
		return;
	}

	AMagicTourGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMagicTourGameMode>() : nullptr;
	if (!GameMode || !GameMode->ReachCheckpoint(Index, NextStage))
	{
		return;
	}

	isComplete = true;
	Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetVisibility(false, true);
}
