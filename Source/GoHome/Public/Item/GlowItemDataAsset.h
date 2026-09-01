

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemDataAsset.h"
#include "GlowItemDataAsset.generated.h"

// Omni 발광 아이템 전용 데이터.
// 손전등(Spot, UFlashlightDataAsset)과는 별도 클래스로 분리함.
// 부착 위치/입력/활성슬롯 방식이 근본적으로 다른 아이템이라, 
// 하나의 클래스 + FixtureType 분기로 묶는 대신 클래스 자체를 나눔.

UCLASS(BlueprintType)
class GOHOME_API UGlowItemDataAsset : public UItemDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, Category = "GlowItem")
	float Intensity = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "GlowItem", meta = (ToolTip = "빛이 닿는 최대 거리"))
	float AttenuationRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "GlowItem", meta = (ToolTip = "물속 등 볼류메트릭 안개와 상호작용하는 정도 - 높을수록 허공에서도 빛줄기 자체가 은은하게 보임"))
	float VolumetricScatteringIntensity = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "GlowItem")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "GlowItem", meta = (ToolTip = "총 점등 가능 시간(초). 켜져 있는 동안만 소모, 다 쓰면 재점등 불가능한 1회용 소모품"))
	float GlowChargeDuration = 15.f;
};
