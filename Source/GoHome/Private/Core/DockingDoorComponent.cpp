

#include "Core/DockingDoorComponent.h"
#include "Net/UnrealNetwork.h"

UDockingDoorComponent::UDockingDoorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UDockingDoorComponent::IsOpen() const
{
	return bOpen;
}

void UDockingDoorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDockingDoorComponent, bOpen);
}

void UDockingDoorComponent::OnRep_bOpen()
{
	OnDoorStateChanged.Broadcast(bOpen);
}
