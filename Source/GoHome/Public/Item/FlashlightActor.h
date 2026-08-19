

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

protected:

	virtual void BeginPlay() override;
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

private:

	// ItemData(UFlashlightDataAsset)의 FixtureType/밝기/각도/색을 실제 라이트 컴포넌트에 반영.
	void SyncLightFromData();
	void UpdateLightVisual();

	// SyncLightFromData가 골라놓은, 지금 사용 중인 라이트 컴포넌트(SpotLight 또는 PointLight).
	UPROPERTY()
	TObjectPtr<USceneComponent> ActiveLight;
};
