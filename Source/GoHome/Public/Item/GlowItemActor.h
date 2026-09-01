

#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "GlowItemActor.generated.h"

class UPointLightComponent;

// Omni(램프형) 발광 아이템 - 오른손 활성 슬롯에서 좌클릭으로 켜고 끔.
// 총 충전량(UGlowItemDataAsset::GlowChargeDuration) 안에서만 재사용 가능한 1회성 소모품.
// 켜진 채로 드롭되면 충전량이 다 소진될 때 까지 계속 켜진 채로 부유함.

UCLASS()
class GOHOME_API AGlowItemActor : public AUsableItemBase
{
	GENERATED_BODY()
	
public:

	AGlowItemActor();

	virtual void ServerUseSpecialAction() override;

	virtual bool CanUse() const override;

	virtual bool IsDeliverable() const override { return false; }

	// 인벤토리 무게는 0이지만, 물속에서 가라앉음.
	virtual float GetBuoyancyWeight() const override { return 2.f; }

protected:

	virtual void BeginPlay() override;
	virtual void OnRep_ItemData() override;
	virtual void UpdateAttachment(APawn* OldHoldingPawn = nullptr) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "GlowItem")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(ReplicatedUsing = OnRep_IsOn)
	bool bIsOn = false;

	UFUNCTION()
	void OnRep_IsOn();

	// 충전량이 얼마 안남았는지(FlickerThreshold 이하) -> 클라이언트가 이 값을 받으면,
	// 정확한 프레임 동기화 없이 각자 로컬에서 깜빡임 연출 시작.
	UPROPERTY(ReplicatedUsing = OnRep_IsFlickering)
	bool bIsFlickering = false;

	UFUNCTION()
	void OnRep_IsFlickering();

private:

	void SyncLightFromData();
	void UpdateLightVisual();

	// 남은 점등 가능 시간(초). bIsOn인 동안만 줄어둠. 0이 되면 완전 소모(재점등 불가).
	// -1은 SyncLightFromData에서 아직 초기화 안된 상태를 뜻함(최초 1회만 DA 값으로 세팅하기 위한 구분용).
	float RemainingGlowCharge = -1.f;

	// 충전량이 이 값 이하로 남으면 깜빡임 연출 시작.
	UPROPERTY(EditAnywhere, Category = "GlowItem")
	float FlickerThreshold = 3.f;

	// 충전량 감소는 Tick이 아닌 타이머로 처리함.
	// -> 드롭된 아이템이 바닥에 정지하면 부력 시스템이 Tick 자체를 꺼버리기 때문(SetActorTickEnabled(false)).
	FTimerHandle GlowChargeTimerHandle;
	void TickGlowCharge();

	void UpdateGlowChargeTimer();

	// 깜빡임 연출용 로컬(클라이언트마다 각자) 타이머.
	FTimerHandle FlickerTimerHandle;
	void ToggleFlickerVisual();


};
