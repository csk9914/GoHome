

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemDataAsset.h"
#include "FlashlightDataAsset.generated.h"

// Spot 손전등 전용 데이터.
// (이전 Omni 램프형도 이 클래스가 FixtureType으로 겸했으나, 부착 위치/입력/활성슬롯 방식이
// 근본적으로 달라져서 Omni는 UGlowItemDataAsset으로 분리됨.)
// Weight / Value는 보통 0으로 둔다(운반/정산 대상 아님, 슬롯만 점유).

UCLASS(BlueprintType)
class GOHOME_API UFlashlightDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	float Intensity = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "원뿔 각도"))
	float OuterConeAngle = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "중심부(완전 밝은 영역) 각도. OuterConeAngle보다 작아야 그 사이가 부드럽게 퍼짐"))
	float InnerConeAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "빛이 닿는 최대 거리"))
	float AttenuationRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "물속 등 볼류메트릭 안개와 상호작용하는 정도 - 높을수록 허공에서도 빛줄기 자체가 은은하게 보임"))
	float VolumetricScatteringIntensity = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	FLinearColor LightColor = FLinearColor::White;

};
