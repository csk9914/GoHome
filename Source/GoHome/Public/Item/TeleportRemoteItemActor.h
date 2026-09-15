

#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "TeleportRemoteItemActor.generated.h"

class ATeleportStationActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeleportChannelChanged, bool, bChanneling);

// 오른손 사용 아이템 : 좌클릭을 꾹 눌러 채널링, 100% 도달 시 잠수정의 텔레포트 기계 앞으로 자동 이동
// 도중 떼거나 슬롯을 바꾸거나 드롭하면 취소, 1회용이라 성공하면 소진
UCLASS()
class GOHOME_API ATeleportRemoteItemActor : public AUsableItemBase
{
	GENERATED_BODY()

public:
	// 좌클릭 눌림 -> 채널링 시작
	virtual void ServerUseSpecialAction() override;

	// 좌클릭 뗌 -> 채널링 취소
	virtual void ServerCancelSpecialAction() override;

	// 소진됐거나 이미 채널링 중이면 false
	virtual bool CanUse() const override;

	// 장비류 - 납품 정산 대상 아님
	virtual bool IsDeliverable() const override { return false; }

	// 홀드 게이지 위젯이 구독. 시작 종료(성공 취소 모두)에 브로드캐스트 됨
	UPROPERTY(BlueprintAssignable, Category = "Teleport")
	FOnTeleportChannelChanged OnChannelChanged;
	
	UFUNCTION(BlueprintPure, Category = "Teleport")
	bool IsChanneling() const { return ChannelEndServerTime > 0.f; }

	// 게이지용 0.0 ~ 1.0 진행률. 채널링 주잉 아니면 0
	UFUNCTION(BlueprintPure, Category = "Teleport")
	float GetChannelProgress() const;

	// 숫자로 남은 시간을 띄우고 싶을 때. 채널링 중이 아니면 0
	UFUNCTION(BlueprintPure, Category = "Teleport")
	float GetChannelRemainingSeconds() const;

	// 홀드 지속 시간. 이 값 하나만 바꾸면 게이지 차는 속도도 같이 바뀜
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport", meta = (ClampMin = "0.1", UIMin = "0.5", UIMax = "10.0", Tooltip = "좌클릭을 꾹 눌러야 하는 시간(초). 게이지 차는 속도가 여기에 맞춰 자동으로 결정됨"))
	float HoldDuration = 3.f;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 손에서 벗어나면 채널링을 취소
	virtual void UpdateAttachment(APawn* OldHoldingPawn = nullptr) override;

	// 채널 시작 시 각 클라에서 호출, bIsLocalUser면 연출을 BP에서 켬
	UFUNCTION(BlueprintImplementableEvent, Category = "Teleport")
	void OnChannelStartedCosmetic(bool bIsLocalUser);

	// 채널 종료시 호출. 연출을 끔
	UFUNCTION(BlueprintImplementableEvent, Category = "Teleport")
	void OnChannelEndedCosmetic(bool bIsLocalUser);

private:
	// 채널 완료 서버 시각(절대값). 0이면 비채널링
	UPROPERTY(ReplicatedUsing = OnRep_ChannelEndServerTime)
	float ChannelEndServerTime = 0.f;

	UFUNCTION()
	void OnRep_ChannelEndServerTime();

	// 1회용 - 성공 시 true;
	UPROPERTY(Replicated)
	bool bConsumed = false;

	// 서버 전용. 만료되면 자동으로 텔레포트 나감
	FTimerHandle ChannelTimerHandle;

	void OnChannelComplete();

	// 서버 전용 : 타이머 해제 + 상태 리셋 + 연출 끄기 공통 처리
	void ClearChannel();

	// 레벨에 배치된 텔레포트 기계를 찾음(첫 1회 후 캐시)
	ATeleportStationActor* FindStation();

	UPROPERTY()
	TObjectPtr<ATeleportStationActor> CachedStation;

	float GetNowServerTime() const;

	// 이 화면의 플레이어가 지금 이걸 들고 있는가(1인칭 연출 판정용)
	bool IsLocallyHeld() const;

	// 연출 중복 실행 방지용 로컬 플래그
	bool bCosmeticActive = false;

	// 1회용 소모처리(인벤토리 제거 + 소멸). 연출 종료가 클라에 도달한 뒤 실행
	void ConsumeSelf();
};
