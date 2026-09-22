


#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "RadarItemActor.generated.h"

class URadarSensorComponent;

// 휴대용 수중 레이더. 실제 탐지 로직은 전부 URadarSensorComponent가 갖는다.
// 이 클래스는 "인벤토리에 들어가는 장비"라는 성격만 정의한다.
UCLASS()
class GOHOME_API ARadarItemActor : public AUsableItemBase
{
	GENERATED_BODY()

public:

	ARadarItemActor();

	// 손전등과 동일 - 슬롯은 차지하지만 납품 정산 대상은 아니다.
	virtual bool IsDeliverable() const override { return false; }

	// 인벤토리 무게와 별개로, 물속에서는 고체 장비라 가라앉는다.
	virtual float GetBuoyancyWeight() const override { return 2.f; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	TObjectPtr<URadarSensorComponent> RadarSensor;
};