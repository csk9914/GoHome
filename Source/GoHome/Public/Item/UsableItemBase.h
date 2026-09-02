

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActorBase.h"
#include "UsableItemBase.generated.h"

// 오른손 활성 슬롯 아이템 중 "사용(좌클릭)" 가능한 것들의 공통 베이스.
// 부착/활성 슬롯 로직은 AItemActorBase 재사용.
// 여기서는 "지금 사용 가능한 상태인지" 판단 지점만 추가함.
// (쿨다운/충전량/탄약 등 구체적인 제약은 서브클래스에 CanUse()를 오버라이드 해서 구현.
//
// 다 쓴 채로 드롭돼서 바닥에 정착하면(AItemActorBase::OnSettled()),
// 일정 시간 후 경고 연출(기본: 메쉬 On/Off 깜빡임)을 거쳐 자동 소멸.

UCLASS()
class GOHOME_API AUsableItemBase : public AItemActorBase
{
	GENERATED_BODY()

public:

	// 지금 사용(좌클릭) 가능한 상태인지 확인.
	// 기본은 true -> 쿨다운/충전량 있는 아이템만 오버라이드.
	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool CanUse() const { return true; }

protected:

	virtual void UpdateAttachment(APawn* OldHoldingPawn = nullptr) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 바닥에 정착하는 순간 호출(AItemActorBase 훅). 다 쓴 상태면 소멸 카운트다운 시작.
	virtual void OnSettled() override;

	// 소멸 임박(경고 연출 시작) 시 호출. 기본은 메쉬를 On/Off로 깜빡임(재질 작업 없이 코드만으로 가능한 방식).
	// 다른 연출이 필요한 서브클래스는 오버라이드해서 교체 가능.
	virtual void OnDespawnImminent();

	// 카운트다운/경고 도중 다시 주워져서 취소될 때 호출. 기본은 깜빡임 정지 + 다시 보이게 복구.
	virtual void OnDespawnCanceled();

private:

	void StartDespawnWarning();
	void DespawnIfDepleted();
	void CancelDespawnTimer();
	void ToggleMeshFlicker();

	// 정착 후 다 쓴 상태면, 이 시간(초) 지나면 경고 연출 시작.
	UPROPERTY(EditAnywhere, Category = "Item")
	float DespawnDelayAfterSettled = 15.f;

	// 경고 연출 시작 후 실제 소멸까지 남은 시간(초).
	UPROPERTY(EditAnywhere, Category = "Item")
	float DespawnWarningLeadTime = 3.f;

	FTimerHandle DespawnTimerHandle;

	// 소멸 경고 중인지 - 클라이언트도 각자 로컬에서 깜빡임을 재생해야 하므로 리플리케이트.
	UPROPERTY(ReplicatedUsing = OnRep_DespawnImminent)
	bool bDespawnImminent = false;

	UFUNCTION()
	void OnRep_DespawnImminent();

	// 깜빡임 연출용 로컬(클라이언트마다 각자) 타이머.
	FTimerHandle MeshFlickerTimerHandle;

};