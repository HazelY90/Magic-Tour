#include "Combat/CastNotify.h"

#include "Characters/MagicTourCharacter.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Components/SkeletalMeshComponent.h"

void UCastNotify::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& Event)
{
	Super::Notify(Mesh, Animation, Event);
	if (Mesh)
	{
		if (AMagicTourCharacter* Character = Cast<AMagicTourCharacter>(Mesh->GetOwner()))
		{
			// Queued notifications from a canceled instance must not release a new action.
			if (const auto* Context = Event.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>())
			{
				Character->ReleaseSpell(Animation, Context->MontageInstanceID);
			}
		}
	}
}
