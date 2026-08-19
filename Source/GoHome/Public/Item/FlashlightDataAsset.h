

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

	UPROPERTY(EditDefaultsOnly, Category = "Flashlight", meta = (ToolTip = "빛이 닿는 최대 거리(두 타입 공통)"))
	float AttenuationRadius = 800.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Flashlight")
	FLinearColor LightColor = FLinearColor::White;

};
