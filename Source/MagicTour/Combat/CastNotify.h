#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CastNotify.generated.h"

/** Releases the queued spell at the raised-hand frame of the active montage. */
UCLASS()
class MAGICTOUR_API UCastNotify : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& Event) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("Release Spell"); }
};
