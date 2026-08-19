
#include "Interaction/MultiActorGateComponent.h"
#include "Interaction/CoopLeverActor.h"
#include "Core/DockingDoorComponent.h"

UMultiActorGateComponent::UMultiActorGateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}



void UMultiActorGateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CachedDoor = GetOwner()->FindComponentByClass<UDockingDoorComponent>();

	if (LeverA)
	{
		LeverA->OnLeverActiveChanged.AddDynamic(this, &UMultiActorGateComponent::OnLeverStateChanged);
	}
	
	if (LeverB)
	{
		LeverB->OnLeverActiveChanged.AddDynamic(this, &UMultiActorGateComponent::OnLeverStateChanged);
	}
}


void UMultiActorGateComponent::OnLeverStateChanged(bool bNewActive)
{
	EvaluateGateState();
}

void UMultiActorGateComponent::EvaluateGateState()
{
	if (!CachedDoor) return;

	const bool bBothActive = LeverA && LeverB && LeverA->IsActive() && LeverB->IsActive();
	CachedDoor->SetOpen(bBothActive);
}
