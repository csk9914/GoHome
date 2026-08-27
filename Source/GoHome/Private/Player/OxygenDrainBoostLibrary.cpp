#include "Player/OxygenDrainBoostLibrary.h"
#include "Player/OxygenComponent.h"
#include "GameFramework/Actor.h"

void UOxygenDrainBoostLibrary::StartOxygenDrainBoostFromInstigator(AActor* TargetActor, AActor* InstigatorActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UOxygenComponent* OxygenComponent = TargetActor->FindComponentByClass<UOxygenComponent>();
	if (!OxygenComponent)
	{
		return;
	}

	OxygenComponent->StartOxygenDrainBoostFromInstigator(InstigatorActor);
}

void UOxygenDrainBoostLibrary::StopOxygenDrainBoostFromInstigator(AActor* TargetActor, AActor* InstigatorActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UOxygenComponent* OxygenComponent = TargetActor->FindComponentByClass<UOxygenComponent>();
	if (!OxygenComponent)
	{
		return;
	}

	OxygenComponent->StopOxygenDrainBoostFromInstigator(InstigatorActor);
}