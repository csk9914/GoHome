


#include "Interaction/ElectricBreakerActor.h"
#include "Interaction/ElectricSwitchboardActor.h"
#include "Components/StaticMeshComponent.h"

AElectricBreakerActor::AElectricBreakerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BreakerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BreakerMesh"));
	SetRootComponent(BreakerMesh);
}


bool AElectricBreakerActor::CanInteract(APawn* InstigatorPawn) const
{
	return LinkedSwitchboard != nullptr;
}

void AElectricBreakerActor::OnInteract(APawn* InstigatorPawn)
{
	if (LinkedSwitchboard)
	{
		LinkedSwitchboard->TryResolveViaBreaker();
	}
}

FText AElectricBreakerActor::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("차단기 내리기"));
}
