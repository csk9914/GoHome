//

#pragma once

#include "CoreMinimal.h"
#include "GoHomeGameState.h"
#include "Data/FSettlementResult.h"
#include "ExplorationGameState.generated.h"

class UHealthComponent;

// 정산 결과가 클라에 도착(호스트는 서버에서 직접)했을 때 브로드캐스트 — 정산/실패 UI가 바인딩
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSettlementReady, const FSettlementResult&, Result);

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

	void SetExpeditionDeadline(float InDeadlineServerTime);
	
	// 서버 전용. FinalizeRound 결과를 복제 필드에 싣고 호스트 로컬에도 즉시 알린다.
	void SetSettlementResult(const FSettlementResult& InResult);

	// BlueprintPure 설정 위젯이 델리게이트 콜백 안에서 값을 꺼내 쓸 수 있음
	UFUNCTION(BlueprintPure, Category = "Expedition")
	const FSettlementResult& GetSettlementResult() const { return SettlementResult; }


protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_SettlementResult();

public:
	// 정산/실패 UI가 바인딩 (state 델리게이트와 별개 — 아래 주석 참고)
	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnSettlementReady OnSettlementReady;
	
private:
	UPROPERTY(Replicated)
	float ExpeditionDeadline = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_SettlementResult, BlueprintReadOnly, Category = "Expedition", meta = (AllowPrivateAccess = "true"))
	FSettlementResult SettlementResult;

};
