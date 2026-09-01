

#include "Core/GoHomeGameState.h"
#include "Core/DockingDoorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Save/GoHomeSaveSubsystem.h"

AGoHomeGameState::AGoHomeGameState()
{
	DockingDoorComponent = CreateDefaultSubobject<UDockingDoorComponent>(TEXT("DockingDoorComponent"));
}

void AGoHomeGameState::SetState(EExpeditionState NewState)
{
	
	CurrentState = NewState;
	OnStateChanged.Broadcast((CurrentState));
	
	// UE_LOG(LogTemp, Warning, TEXT("AGoHomeGameState::SetState(): Class=%s CurrentState=%s"), *GetClass()->GetName(), *UEnum::GetValueAsString(CurrentState));
}

void AGoHomeGameState::AddDeliveredValue(int32 Value)
{
	if (!HasAuthority())
	{
		return;
	}
	
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}
	
	UGoHomeSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UGoHomeSaveSubsystem>();
	if (!SaveSubsystem)
	{
		return;
	}
	
	SaveSubsystem->AccumulateDeliveredValue(Value);
	
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

	DOREPLIFETIME(AGoHomeGameState, CurrentState);
}

void AGoHomeGameState::OnRep_State()
{
	OnStateChanged.Broadcast(CurrentState);
}
