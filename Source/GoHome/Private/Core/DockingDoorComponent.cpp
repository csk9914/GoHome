

#include "Core/DockingDoorComponent.h"
#include "Net/UnrealNetwork.h"

UDockingDoorComponent::UDockingDoorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDockingDoorComponent::SetOpen(bool bNewOpen)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (bOpen == bNewOpen)
	{
		return;
	}

	bOpen = bNewOpen;

	// OnRep은 서버 자신에게는 자동 호출되지 않으므로 직접 호출해 서버(호스트) 쪽 구독자도 갱신되게 함
	OnRep_bOpen();
}

void UDockingDoorComponent::OnRep_bOpen()
{
	OnDoorStateChanged.Broadcast(bOpen);
}


void UDockingDoorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDockingDoorComponent, bOpen);
}
	
