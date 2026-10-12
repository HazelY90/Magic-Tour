#include "World/CombatArena.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"

ACombatArena::ACombatArena()
{
	Tags.Add(TEXT("CombatArena"));
}

bool ACombatArena::IsInside(FVector Pos) const
{
	const FVector Offset = Pos - GetActorLocation();
	return !Pos.ContainsNaN() && FMath::IsFinite(Radius) && FMath::IsFinite(HalfHeight)
		&& Radius > 0.0f && HalfHeight > 0.0f
		&& Offset.SizeSquared2D() <= FMath::Square(Radius) && FMath::Abs(Offset.Z) <= HalfHeight;
}

bool ACombatArena::IsPathInside(const TArray<FVector>& Points) const
{
	if (Points.Num() < 2) { return false; }
	for (const FVector& Pos : Points)
	{
		if (!IsInside(Pos)) { return false; }
	}
	// A disk is convex, so segments between accepted path points remain inside too.
	return true;
}

bool ACombatArena::IsPatrolFound(const FVector& From, FVector& Pos) const
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav || !IsInside(From)) { return false; }
	for (int32 Try = 0; Try < 16; ++Try)
	{
		FNavLocation Point;
		if (Nav->GetRandomReachablePointInRadius(From, Radius * 2.0f, Point)
			&& IsInside(Point.Location)
			&& FVector::DistSquared2D(Point.Location, GetActorLocation()) < FMath::Square(Radius - 100.0f)
			&& FVector::DistSquared2D(Point.Location, From) > FMath::Square(200.0f))
		{
			UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
				GetWorld(), From, Point.Location);
			if (Path && Path->IsValid() && !Path->IsPartial() && IsPathInside(Path->PathPoints))
			{
				Pos = Point.Location;
				return true;
			}
		}
	}
	return false;
}
