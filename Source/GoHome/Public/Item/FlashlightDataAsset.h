

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemDataAsset.h"
#include "FlashlightDataAsset.generated.h"

UENUM(BlueprintType)
enum class ELightFixtureType : uint8
{
	Spot, // 정면 원뿔형 (일반 손전등)
	Omni, // 사방 확산 (램프)
};

// 손전등 종류별 라이트 설정.
// Weight / Value는 보통 0으로 둔다(운반/정산 대상 아님, 슬롯만 점유).
UCLASS(BlueprintType)
class GOHOME_API UFlashlightDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	ELightFixtureType FixtureType = ELightFixtureType::Spot;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	float Intensity = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "Spot 타입 전용 - 원뿔 각도"))
	float OuterConeAngle = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "Spot 타입 전용 - 중심부(완전히 밝은 영역) 각도. OuterConeAngle보다 작아야 그 사이가 부드럽게 퍼짐"))
	float InnerConeAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "빛이 닿는 최대 거리(두 타입 공통)"))
	float AttenuationRadius = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "물속 등 볼류메트릭 안개와 상호작용하는 정도(두 타입 공통) - 높을수록 허공에서도 빛줄기 자체가 은은하게 보임"))
	float VolumetricScatteringIntensity = 1.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "Omni 타입 전용 - 총 점등 가능 시간(초). 켜져 있는 동안만 소모, 다 쓰면 재점등 불가능한 1회용 소모품"))
	float GlowChargeDuration = 15.f;

};
