

#include "Core/GoHomeGameState.h"
#include "Core/DockingDoorComponent.h"
#include "Core/ExplorationGameMode.h"
#include "Core/ExplorationGameState.h"
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
	
	const int32 RoundDeliveredTotal = SaveSubsystem->AccumulateDeliveredValue(Value);

	// 호스트 전용 세이브 값을 탐사 GameState의 복제 미러로 밀어 라이브 HUD(할당량·자금)가 받게 한다
	if (AExplorationGameState* ExplorationGameState = Cast<AExplorationGameState>(this))
	{
		// 자금을 먼저 실어야 SetRoundDeliveredValue의 호스트 브로드캐스트 시점에 최신 자금이 보인다
		ExplorationGameState->SetCurrentFunds(SaveSubsystem->GetCurrentFunds());
		ExplorationGameState->SetRoundDeliveredValue(RoundDeliveredTotal);
	}
}

void AGoHomeGameState::Fail(EFailReason Reason)
{
	if (!HasAuthority())
	{
		return;
	}

	// GameMode는 서버 전용, 클라는 null
	if (AExplorationGameMode* ExplorationGameMode = GetWorld()->GetAuthGameMode<AExplorationGameMode>())
	{
		ExplorationGameMode->HandleFail(Reason);
	}

	
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
