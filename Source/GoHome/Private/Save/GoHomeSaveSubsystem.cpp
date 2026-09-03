#include "Save/GoHomeSaveSubsystem.h"
#include "Save/GoHomeSaveGame.h"
#include "Core/ExpeditionState.h"
#include "Kismet/GameplayStatics.h"
#include "Core/GoHomeGameState.h"
#include "Data/EconomyConfigDataAsset.h"
#include "Engine/World.h"

namespace
{
	const FString GoHomeSaveSlotName = TEXT("GoHomeSave");
	constexpr int32 GoHomeSaveUserIndex = 0;
}

void UGoHomeSaveSubsystem::Initialize(FSubsystemCollectionBase& CollectionBase)
{
	Super::Initialize(CollectionBase);

	// 디스크에 그 이름의 세이브 파일이 실제로 있는지 확인
	if (UGameplayStatics::DoesSaveGameExist(GoHomeSaveSlotName, GoHomeSaveUserIndex))
	{
		SaveGame = Cast<UGoHomeSaveGame>(UGameplayStatics::LoadGameFromSlot(GoHomeSaveSlotName, GoHomeSaveUserIndex));
	}

	// 파일이 없거나, 캐스트에 실패했을 경우
	if (!SaveGame)
	{
		// 새로운 빈 SaveGame 인스턴스를 만듬
		SaveGame = Cast<UGoHomeSaveGame>(UGameplayStatics::CreateSaveGameObject(UGoHomeSaveGame::StaticClass()));
	}

	EconomyConfig = LoadObject<UEconomyConfigDataAsset>(nullptr, TEXT("/Game/GoHome/Data/DA_EconomyConfig"));
	ensureMsgf(EconomyConfig, TEXT("DA_EconomyConfig 로드 실패 - 경로 확인"));

	// FCoreUObjectDelegates::PostLoadMapWithWorld : 엔진이 맵 로드를 끌낼 때마다 전역으로 쏘는 델리게이트
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UGoHomeSaveSubsystem::OnPostLoadMap);
}

void UGoHomeSaveSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	Super::Deinitialize();
}

void UGoHomeSaveSubsystem::SaveToDisk()
{
	if (!SaveGame)
	{
		return;
	}

	UGameplayStatics::SaveGameToSlot(SaveGame, GoHomeSaveSlotName, GoHomeSaveUserIndex);
}

int32 UGoHomeSaveSubsystem::AccumulateDeliveredValue(int32 Value)
{
	if (!SaveGame)
	{
		return 0;
	}

	// 탐사중인 미션 할당량 반영
	SaveGame->CurrentRoundDeliveredValue += Value;

	// 소유 자금에 납품액 반영
	SaveGame->CurrentFunds += Value;

	return SaveGame->CurrentRoundDeliveredValue;
}

FSettlementResult UGoHomeSaveSubsystem::FinalizeRound(bool bForfeited, const TArray<FString>& CasualtyNames)
{
	if (!SaveGame || !EconomyConfig)
	{
		return FSettlementResult();
	}

	FSettlementResult Result;

	// 이번 턴 납품액 확정
	const int32 RoundDeliveredValue = SaveGame->CurrentRoundDeliveredValue;
	const int32 EffectiveDelivered = bForfeited ? 0 : RoundDeliveredValue;

	// forfeit 면 계산 롤백
	if (bForfeited)
	{
		SaveGame->CurrentFunds -= RoundDeliveredValue;
	}

	// 사망 패널티
	const int32 CasualtyPenalty = CasualtyNames.Num() * EconomyConfig->CasualtyFee;
	SaveGame->CurrentFunds -= CasualtyPenalty;

	// 실질적인 획득량
	const int32 NetGain = EffectiveDelivered - CasualtyPenalty;

	// 라운드 완료
	const int32 CompletedRound = ++(SaveGame->CurrentRound);

	// 스트라이크 판정
	if (EffectiveDelivered < CurrentMapQuota)
	{
		SaveGame->QuotaMissCount++;
	}

	// 체크포인트 판정
	const FCheckPoint* CheckPoint = EconomyConfig->FindCheckPoint(CompletedRound);
	ESettlementOutcome Outcome = DetermineOutcome(CheckPoint, CompletedRound);
	
	// 결과 스냅샷
	Result.bForfeited = bForfeited;
	Result.Outcome = Outcome;
	Result.RoundDeliveredValue = RoundDeliveredValue;
	Result.MapQuota = CurrentMapQuota;
	Result.CasualtyNames = CasualtyNames;
	Result.CasualtyPenalty = CasualtyPenalty;
	Result.NetGain = NetGain;
	Result.bWasCheckPoint = CheckPoint != nullptr;
	Result.CheckPointQuota = CheckPoint ? CheckPoint->TargetQuota : 0;
	Result.ExpeditionProgress = BuildProgress();

	// 상태 반영
	const bool bTerminal =
		Outcome == ESettlementOutcome::GameOver_Strike ||
		Outcome == ESettlementOutcome::GameOver_CheckPoint ||
		Outcome == ESettlementOutcome::Ending;

	if (bTerminal)
	{
		ResetSave();
	}
	else
	{
		SaveGame->CurrentRoundDeliveredValue = 0;
	}
	
	// 트래블 전에 디스크에 남긴다
	SaveToDisk();

	// 다음 라운드 출발 때 다시 세팅되므로 필수는 아니지만, 0으로 초기화
	CurrentMapQuota = 0;
	
	return Result;
}

FExpeditionProgress UGoHomeSaveSubsystem::BuildProgress() const
{
	if (!SaveGame)
	{
		return FExpeditionProgress();
	}

	FExpeditionProgress Progress;
	Progress.CurrentRound = SaveGame->CurrentRound;
	Progress.CurrentFunds = SaveGame->CurrentFunds;
	Progress.StrikeCount = SaveGame->QuotaMissCount;

	if (EconomyConfig)
	{
		Progress.FinalRound = EconomyConfig->GetFinalRound();

		if (const FCheckPoint* Next = EconomyConfig->FindNextCheckPoint(SaveGame->CurrentRound))
		{
			Progress.NextCheckPointRound = Next->Round;
			Progress.NextCheckPointQuota = Next->TargetQuota;
		}
	}

	return Progress;
}

void UGoHomeSaveSubsystem::ResetSave()
{
	// 깔끔하게 새로 만들어서 기본값으로 초기화 
	SaveGame = Cast<UGoHomeSaveGame>(UGameplayStatics::CreateSaveGameObject(UGoHomeSaveGame::StaticClass()));
}

ESettlementOutcome UGoHomeSaveSubsystem::DetermineOutcome(const FCheckPoint* CheckPoint, int32 CompletedRound) const
{
	if (SaveGame->QuotaMissCount >= 3)
		return ESettlementOutcome::GameOver_Strike;
	if (CheckPoint && SaveGame->CurrentFunds < CheckPoint->TargetQuota)
		return ESettlementOutcome::GameOver_CheckPoint;
	if (CheckPoint && EconomyConfig->IsEndingRound(CompletedRound))
		return ESettlementOutcome::Ending;
	if (CheckPoint)
		return ESettlementOutcome::CheckPointPassed;
	return ESettlementOutcome::Normal;
}

void UGoHomeSaveSubsystem::OnExpeditionStateChanged(EExpeditionState NewState)
{
	if (NewState == EExpeditionState::Lobby)
	{
		SaveToDisk();
	}
}

void UGoHomeSaveSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld != GetGameInstance()->GetWorld() || LoadedWorld->GetNetMode() == NM_Client)
	{
		return;
	}

	AGoHomeGameState* GameState = LoadedWorld->GetGameState<AGoHomeGameState>();
	if (!GameState)
	{
		return;
	}

	GameState->OnStateChanged.AddUniqueDynamic(this, &UGoHomeSaveSubsystem::OnExpeditionStateChanged);

	if (GameState->GetCurrentState() == EExpeditionState::Lobby)
	{
		SaveToDisk();
	}
}
