#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Upgrade/EquipmentUpgradeTypes.h"
#include "EquipmentUpgradeDataAsset.generated.h"

class UTexture2D;

// 강화 레벨 한 줄의 데이터
// 예: 산소 강화 2레벨 = 결과값 18, 비용 150
USTRUCT(BlueprintType)
struct GOHOME_API FEquipmentUpgradeLevelDefinition
{
	GENERATED_BODY()

	// 이 줄이 몇 레벨 데이터인지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Upgrade", meta = (ClampMin = "1"))
	int32 Level = 1;

	// 강화 후 실제 적용될 보너스 값
	// 예: 산소 +3칸, 무게 +2kg
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Upgrade")
	float BonusValue = 0.f;

	// UI에 보여줄 값이 따로 필요할 때 사용
	// 비워두면 ResultValue로 자동 표시 가능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Upgrade")
	FText ResultValueTextOverride;

	// 현재 레벨에서 이 레벨로 올릴 때 필요한 비용
	// 예: 1레벨 -> 2레벨 비용 150
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Upgrade", meta = (ClampMin = "0"))
	int32 CostToReachLevel = 0;
};

// 강화 하나의 전체 데이터 에셋
// 예: 산소 탱크 업그레이드 1개, 배낭 무게 업그레이드 1개
UCLASS(BlueprintType)
class GOHOME_API UEquipmentUpgradeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 강화 고유 이름표
	// 예: OxygenCapacity, CarryWeightLimit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Upgrade")
	FName UpgradeId = NAME_None;

	// 이 강화가 어떤 능력치를 올리는지
	// 예: 산소 최대치 / 무게 한도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Upgrade")
	EEquipmentUpgradeEffectType EffectType = EEquipmentUpgradeEffectType::None;

	// UI에 표시할 강화 이름
	// 예: 산소 탱크 업그레이드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Upgrade")
	FText UpgradeName;

	// UI에 표시할 능력치 이름
	// 예: 용량, 무게
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Upgrade")
	FText StatLabel;

	// UI에 붙일 단위.
	// 예: 산소는 "칸", 무게는 " kg"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Upgrade")
	FText ValueUnitText;

	// UI 카드에 보여줄 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Upgrade")
	TObjectPtr<UTexture2D> Icon;

	// 레벨별 강화표
	// 예: 1레벨 15칸, 2레벨 18칸, 3레벨 20칸
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Upgrade")
	TArray<FEquipmentUpgradeLevelDefinition> LevelDefinitions;

	// 최대 레벨 가져오기
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	int32 GetMaxLevel() const;

	// 잘못된 레벨 값이 들어오면 안전한 범위로 고정
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	int32 GetClampedLevel(int32 Level) const;

	// 현재 레벨이 최대 레벨인지 확인
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	bool IsMaxLevel(int32 CurrentLevel) const;

	// 특정 레벨의 실제 결과값 가져오기
	// 예: 2레벨 산소 = 18
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	float GetBonusValueAtLevel(int32 Level) const;

	// 기본값 + 해당 레벨 보너스 = 실제 표시/적용될 최종값
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	float GetFinalValueAtLevel(int32 Level, float BaseValue) const;

	// 특정 레벨의 UI 표시용 텍스트 가져오기
	// 예: "18칸", "7kg"
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	FText GetFinalValueTextAtLevel(int32 Level, float BaseValue) const;

	// 현재 레벨에서 다음 레벨로 올릴 때 비용 가져오기
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	int32 GetCostToUpgradeFromLevel(int32 CurrentLevel) const;

	// UI 카드에 넣을 표시 데이터를 한 번에 만들어줌
	// 예: 현재값, 다음값, 비용, 현재레벨, 최대레벨
	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	FEquipmentUpgradePreview BuildPreview(int32 CurrentLevel, float BaseValue) const;
};
