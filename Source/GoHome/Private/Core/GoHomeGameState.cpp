

#include "Core/GoHomeGameState.h"
#include "Core/DockingDoorComponent.h"
#include "Net/UnrealNetwork.h"

AGoHomeGameState::AGoHomeGameState()
{
	DockingDoorComponent = CreateDefaultSubobject<UDockingDoorComponent>(TEXT("DockingDoorComponent"));
}

void AGoHomeGameState::AddDeliveredValue(int32 Value)
{
}

void AGoHomeGameState::Fail(EFailReason Reason)
{
}

void AGoHomeGameState::OnPlayerRemovedFromParty(APlayerState* PlayerState)
{
}

void AGoHomeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoHomeGameState, State);
}

void AGoHomeGameState::OnRep_State()
{
	OnStateChanged.Broadcast(State);
}
