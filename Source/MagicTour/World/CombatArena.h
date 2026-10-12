#pragma once

#include "CoreMinimal.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "CombatArena.generated.h"

/** Navigation bounds with a circular gameplay region for the ruins encounter. */
UCLASS()
class MAGICTOUR_API ACombatArena : public ANavMeshBoundsVolume
{
	GENERATED_BODY()

public:
	ACombatArena();

	/** Keep targets and patrol paths inside the ruins, including their vertical extent. */
	UFUNCTION(BlueprintPure, Category="Arena")
	bool IsInside(FVector Pos) const;

	bool IsPathInside(const TArray<FVector>& Points) const;
	bool IsPatrolFound(const FVector& From, FVector& Pos) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Arena", meta=(ClampMin="100.0"))
	float Radius = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Arena", meta=(ClampMin="100.0"))
	float HalfHeight = 600.0f;
};
