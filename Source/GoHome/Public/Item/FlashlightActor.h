

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActorBase.h"
#include "FlashlightActor.generated.h"

class USpotLightComponent;
class UPointLightComponent;

// 손전등 : 인벤토리 슬롯은 차지하지만(운반 용량과 트레이드 오프)
// 활성 슬롯 여부와 무관하게 습득 즉시 왼손에 상시 부착.
// F 키로 온/오프만 토글.
// 라이트 종류(정면/사방)는 ItemData(UFlashlightDataAsset)로 결정 -> 새 종류 추가 시 코드 수정 불필요.

UCLASS()
class GOHOME_API AFlashlightActor : public AItemActorBase
{
	GENERATED_BODY()

public:

	AFlashlightActor();

	virtual void OnInteract(APawn* InstigatorPawn) override;

	virtual FText GetInteractionPromptText_Implementation() const override;

	// InventoryComponent가 호출(서버 권위) - 켜짐/꺼짐 반전.
	virtual void ServerUseSpecialAction() override;
	
	virtual bool IsDeliverable() const override { return false; }

	// 인벤토리 무게는 0이지만, 실제 손전등은 고체 장비라 물속에서 가라앉음.
	virtual float GetBuoyancyWeight() const override { return 2.f; };


protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_ItemData() override;
	virtual void UpdateAttachment(APawn* OldHoldingPawn = nullptr) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> SpotLight;

	UPROPERTY(VisibleAnywhere, Category = "Flashlight")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(ReplicatedUsing = OnRep_IsOn)
	bool bIsOn = false;

	UFUNCTION()
	void OnRep_IsOn();

	// Omni 전용 : 충전량이 얼마 안 남았는지(FlickerThreshold 이하)
	// 클라이언트가 이 값을 받으면 정확한 프레임 동기화 없이 각자 로컬에서 깜빡임 연출 시작.
	UPROPERTY(ReplicatedUsing = OnRep_IsFlickering)
	bool bIsFlickering = false;

	UFUNCTION()
	void OnRep_IsFlickering();

private:

	// ItemData(UFlashlightDataAsset)의 FixtureType/밝기/각도/색을 실제 라이트 컴포넌트에 반영.
	void SyncLightFromData();
	void UpdateLightVisual();

	// SyncLightFromData가 골라놓은, 지금 사용 중인 라이트 컴포넌트(SpotLight 또는 PointLight).
	UPROPERTY()
	TObjectPtr<USceneComponent> ActiveLight;

	// --- Omni(램프) 전용 1회성 충전량 시스템 ---
	// 남은 점등 가능 시간(초). bIsOn인 동안만 줄어든다.
	// 0 되면 완전히 소모(재점등 불가).
	// -1은 SyncLightFromData에서 아직 초기화 안 된 상태를 뜻함(최초 1회만 DA 값으로 세팅하기 위한 구분용).
	float RemainingGlowCharge = -1.f;

	// 충전량이 이 값 이하로 남으면 깜빡임 연출 시작.
	UPROPERTY(EditAnywhere, Category = "Flashlight")
	float FlickerThreshold = 3.f;

	// 충전량 감소는 Tick이 아니라 타이머로 처리함
	// -> 드롭 후 바닥에 정지한 아이템은 부력 시스템이 Tick 자체를 꺼버리기 때문(SetActorTickEnabled(false)).
	// Tick에 의존하면 정지된 램프의 충전량이 안 줄어드는 버그 발생.
	FTimerHandle GlowChargeTimerHandle;
	void TickGlowCharge();

	// bIsOn/FixtureType/남은 충전량 상태에 맞춰 GlowChargeTimerHandle을 시작(또는 정지).
	void UpdateGlowChargeTimer();

	// 깜빡임 연출용 로컬(클라이언트 각자) 타이머.
	FTimerHandle FlickerTimerHandle;
	void ToggleFlickerVisual();


};
