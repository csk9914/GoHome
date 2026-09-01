// 


#include "Core/ExplorationGameMode.h"
#include "Core/ExplorationGameState.h"
#include "Player/DeathNotifier.h"
#include "GameFramework/PlayerState.h"
#include "Save/GoHomeSaveSubsystem.h"


AExplorationGameMode::AExplorationGameMode()
{
	GameStateClass = AExplorationGameState::StaticClass();
}

void AExplorationGameMode::HandleFail(EFailReason Reason)
{
	if (bFailHandled)
	{
		return;
	}
	bFailHandled = true;
	
	if (AGoHomeGameState* GoHomeGameState = GetGameState<AGoHomeGameState>())
	{
		GoHomeGameState->SetState(EExpeditionState::Failed);
	}
	
	if (UGoHomeSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UGoHomeSaveSubsystem>())
	{
		SaveSubsystem->FinalizeRound(/*bForfeited=*/true, CasualtyNames.Array());
	}
	
	GetWorldTimerManager().SetTimer(AutoReturnTimer, this, &AExplorationGameMode::ReturnToLobby,AutoReturnDelay, false );
	
}

void AExplorationGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (NewPlayer)
	{
		if (APawn* Pawn = NewPlayer->GetPawn())
		{
			TrackPawnDeath(Pawn);
		}
	}
}

void AExplorationGameMode::TrackPawnDeath(APawn* Pawn)
{
	if (UActorComponent* NotifierComponent = Pawn->FindComponentByInterface(UDeathNotifier::StaticClass()))
	{
		IDeathNotifier* DeathNotifier = Cast<IDeathNotifier>(NotifierComponent);
		if (!DeathNotifier)
		{
			return;
		}
		
		// 재구독 방지
		if (TrackedNotifiers.Contains(NotifierComponent))
		{
			return;
		}
		
		TrackedNotifiers.Add(NotifierComponent);
		DeathNotifier->GetOnDeathDelegate().AddUObject(this, &AExplorationGameMode::HandlePawnDeath, TWeakObjectPtr<APawn>(Pawn));
		
	}
}

void AExplorationGameMode::HandlePawnDeath(TWeakObjectPtr<APawn> DeadPawn)
{
	APawn* Pawn = DeadPawn.Get();
	if (!Pawn)
	{
		return;
	}

	// 사망 집계
	DeadPawns.Add(Pawn);
	
	// 사망 순간폰이 이미 파괴돼 있으면 이름을 못 얻음
	if (APlayerState* PlayerState = Pawn->GetPlayerState())
	{
		CasualtyNames.Add(PlayerState->GetPlayerName());
	}
	
	CheckAllDead();
}

void AExplorationGameMode::CheckAllDead()
{
	if (bFailHandled)
	{
		return;
	}

	AGameStateBase* GameStateBase = GameState;
	if (!GameStateBase)
	{
		return;
	}

	int32 AliveCount = 0;
	for (APlayerState* PlayerState : GameStateBase->PlayerArray)
	{
		APawn* Pawn = PlayerState->GetPawn();
		if (Pawn && !DeadPawns.Contains(Pawn))
		{
			AliveCount++;
		}
	}

	if (AliveCount == 0)
	{
		HandleFail(EFailReason::AllPlayersDead);
	}
}

void AExplorationGameMode::ReturnToLobby()
{
	ServerTravelViaLoadingScreen(GetLobbyMapPath());
}
