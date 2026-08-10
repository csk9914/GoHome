//THE

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/WeightProvider.h"
#include "Interaction/InventorySlot.h"
#include "InventoryComponent.generated.h"

// UHT가 생성하는 리플렉션 코드가 클래스 본문(GENERATED_BODY 위치)에서 이 상수를 참조하므로,
// 클래스 멤버가 아니라 파일 스코프 상수로 둬야 한다 (클래스 내부 선언 순서와 무관하게 항상 보이도록).
inline constexpr int32 GoHomeInventorySlotCount = 4;

/**
 * 4슬롯 인벤토리. IWeightProvider를 구현해 보유 아이템 무게 합산을 노출한다.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class GOHOME_API UInventoryComponent : public UActorComponent, public IWeightProvider
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// 고정 배열(TArray 아님) 사용 이유:
	// 1) 슬롯 수는 4개로 기획상 고정이며 런타임에 크기가 바뀌지 않는다.
	// 2) "슬롯별 개별 RepNotify" 요구사항은 TArray 전환만으로는 해결되지 않는다
	//    (FFastArraySerializer 또는 슬롯별 개별 프로퍼티 분리가 필요 — 별도 후속 작업).
	// 3) BlueprintReadOnly 노출이 필요해지는 시점(UI 바인딩 착수)에 TArray 전환을 재검토한다.
	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	FInventorySlot Slots[GoHomeInventorySlotCount];

	virtual float GetTotalWeight() const override;

	// 빈 슬롯을 찾아 Item을 채운다. 빈 슬롯이 없으면 false.
	// 서버에서만 호출할 것.
	bool TryAddItem(AItemActorBase* Item);

	// Item이 들어있는 슬롯을 비운다. 찾아서 비웠으면 true.
	// 서버에서만 호출할 것.
	bool RemoveItem(AItemActorBase* Item);


protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Slots();

private:
	int32 FindEmptySlotIndex() const;
};
