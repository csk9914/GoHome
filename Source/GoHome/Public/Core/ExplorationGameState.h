//

#pragma once

#include "CoreMinimal.h"
#include "GoHomeGameState.h"
#include "Data/FSettlementResult.h"
#include "ExplorationGameState.generated.h"

class UHealthComponent;

// 정산 결과가 클라에 도착(호스트는 서버에서 직접)했을 때 브로드캐스트 — 정산/실패 UI가 바인딩
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSettlementReady, const FSettlementResult&, Result);

// 라이브 할당량 진행도(납품 누적액 / 맵 할당량)가 바뀔 때 브로드캐스트 — 상시 HUD가 바인딩
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuotaProgressChanged, int32, DeliveredValue, int32, MapQuota);

// 제한시간 정보(마감 서버시각 + 총 길이)가 복제 도착했을 때 브로드캐스트 — 상시 HUD가 바인딩, 바인딩 직후 Get*()로 초기값 1회
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimeLimitChanged);

/**
 *
 */

UCLASS()
class GOHOME_API AExplorationGameState : public AGoHomeGameState
{
	GENERATED_BODY()

public:
	AExplorationGameState();
	
	// 위젯이 매 틱 호출해 카운트다운 표시 (서버, 클라)
	UFUNCTION(BlueprintPure, Category = "Expedition")
	float GetRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Expedition")
	bool HasTimeLimit()const {return ExpeditionDeadline>0.f;}

	// 총 제한시간(초). 프로그레스 바 비율용. 미설정 시 0.
	UFUNCTION(BlueprintPure, Category = "Expedition")
	float GetTotalSeconds() const { return ExpeditionDurationSeconds; }

	// 서버 전용. 마감 서버시각 + 총 제한시간을 복제 필드에 싣는다(AExplorationGameMode::BeginPlay).
	void SetExpeditionDeadline(float InDeadlineServerTime, float InDurationSeconds);
	
	// 서버 전용. FinalizeRound 결과를 복제 필드에 싣고 호스트 로컬에도 즉시 알린다.
	void SetSettlementResult(const FSettlementResult& InResult);

	// BlueprintPure 설정 위젯이 델리게이트 콜백 안에서 값을 꺼내 쓸 수 있음
	UFUNCTION(BlueprintPure, Category = "Expedition")
	const FSettlementResult& GetSettlementResult() const { return SettlementResult; }

	// 서버 전용. 출발 시 존 데이터에서 읽은 맵 할당량을 복제 필드에 싣는다(AExplorationGameMode::BeginPlay).
	void SetMapQuota(int32 InMapQuota);

	// 서버 전용. 납품 누적 후 세이브의 새 합계를 복제 필드에 싣는다(AGoHomeGameState::AddDeliveredValue).
	void SetRoundDeliveredValue(int32 InDeliveredValue);

	UFUNCTION(BlueprintPure, Category = "Expedition")
	int32 GetMapQuota() const { return MapQuota; }

	UFUNCTION(BlueprintPure, Category = "Expedition")
	int32 GetRoundDeliveredValue() const { return RoundDeliveredValue; }

	// 서버 전용. 납품/정산 후 세이브의 새 보유 자금 합계를 복제 필드에 싣는다(AGoHomeGameState::AddDeliveredValue).
	void SetCurrentFunds(int32 InCurrentFunds);

	// 인게임 HUD가 OnQuotaProgressChanged 콜백 안에서 자금 표시값을 꺼내 쓴다(델리게이트 시그니처는 유지).
	UFUNCTION(BlueprintPure, Category = "Expedition")
	int32 GetCurrentFunds() const { return CurrentFunds; }


protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_SettlementResult();

	UFUNCTION()
	void OnRep_QuotaProgress();

	UFUNCTION()
	void OnRep_ExpeditionTime();

public:
	// 정산/실패 UI가 바인딩 (state 델리게이트와 별개 — 아래 주석 참고)
	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnSettlementReady OnSettlementReady;

	// 상시 할당량 HUD가 바인딩 — 최초 값 반영을 위해 바인딩 직후 Get*()로 한 번 당겨오도록
	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnQuotaProgressChanged OnQuotaProgressChanged;

	// 상시 제한시간 HUD가 바인딩 — 바인딩 직후 Get*()로 한 번 당겨오도록
	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnTimeLimitChanged OnTimeLimitChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_ExpeditionTime)
	float ExpeditionDeadline = 0.f;

	// 총 제한시간(초). 마감시각과 함께 1회 세팅, 바 비율 계산용.
	UPROPERTY(Replicated)
	float ExpeditionDurationSeconds = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_SettlementResult, BlueprintReadOnly, Category = "Expedition", meta = (AllowPrivateAccess = "true"))
	FSettlementResult SettlementResult;

	// 현재 맵의 할당량 기준(출발 시 1회 세팅). 라이브 HUD "납품 / 할당량" 표시용.
	UPROPERTY(ReplicatedUsing = OnRep_QuotaProgress, BlueprintReadOnly, Category = "Expedition", meta = (AllowPrivateAccess = "true"))
	int32 MapQuota = 0;

	// 이번 라운드 납품 누적액(세이브 CurrentRoundDeliveredValue 미러). 납품마다 갱신.
	UPROPERTY(ReplicatedUsing = OnRep_QuotaProgress, BlueprintReadOnly, Category = "Expedition", meta = (AllowPrivateAccess = "true"))
	int32 RoundDeliveredValue = 0;

	// 보유 자금(세이브 CurrentFunds 미러, 세이브는 호스트 전용 SoT). 납품마다 갱신, 인게임 HUD 표시용.
	UPROPERTY(ReplicatedUsing = OnRep_QuotaProgress, BlueprintReadOnly, Category = "Expedition", meta = (AllowPrivateAccess = "true"))
	int32 CurrentFunds = 0;

};
