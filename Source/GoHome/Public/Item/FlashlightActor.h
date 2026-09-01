

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActorBase.h"
#include "FlashlightActor.generated.h"

class USpotLightComponent;

// Spot 손전등 : 인벤토리 슬롯은 차지하지만(운반 용량과 트레이드 오프)
// 활성 슬롯 여부와 무관하게 습득 즉시 가슴(Spine_03)에 상시 부착.
// F 키로 온/오프만 토글. 드롭하면 항상 꺼짐.

UCLASS()
class GOHOME_API AFlashlightActor : public AItemActorBase
{
	GENERATED_BODY()

public:

	AFlashlightActor();

	virtual void OnInteract(APawn* InstigatorPawn) override;

	virtual FText GetInteractionPromptText_Implementation() const override;

	// InventoryComponent가 호출(서버 권위) -> 켜짐/꺼짐 반전.
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

	UPROPERTY(ReplicatedUsing = OnRep_IsOn)
	bool bIsOn = false;

	UFUNCTION()
	void OnRep_IsOn();

private:

	// ItemData(UFlashlightDataAsset)의 밝기/각도/색을 실제 라이트 컴포넌트에 반영.
	void SyncLightFromData();
	void UpdateLightVisual();

};
