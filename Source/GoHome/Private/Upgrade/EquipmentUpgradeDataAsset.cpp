#include "Upgrade/EquipmentUpgradeDataAsset.h"

// 데이터 개수 그대로 반환 (0개면 0)
int32 UEquipmentUpgradeDataAsset::GetMaxLevel() const
{
	return LevelDefinitions.Num();
}

// 레벨 범위 고정 (데이터가 없으면 0)
int32 UEquipmentUpgradeDataAsset::GetClampedLevel(int32 Level) const
{
	const int32 MaxLevel = GetMaxLevel();
	if (MaxLevel <= 0)
	{
		return 0;
	}

	return FMath::Clamp(Level, 1, MaxLevel);
}

// 최대 레벨 도달 여부
bool UEquipmentUpgradeDataAsset::IsMaxLevel(int32 CurrentLevel) const
{
	const int32 MaxLevel = GetMaxLevel();
	if (MaxLevel <= 0)
	{
		return true; // 데이터가 없으면 강화 불가 처리
	}

	return CurrentLevel >= MaxLevel;
}

// 특정 레벨의 보너스 값
// GetBonusValueAtLevel 도 일관되게 GetClampedLevel 적용
float UEquipmentUpgradeDataAsset::GetBonusValueAtLevel(int32 Level) const
{
	const int32 Index = GetClampedLevel(Level) - 1;
	return LevelDefinitions.IsValidIndex(Index) ? LevelDefinitions[Index].BonusValue : 0.f;
}

float UEquipmentUpgradeDataAsset::GetFinalValueAtLevel(int32 Level, float BaseValue) const
{
	return FMath::Max(0.f, BaseValue + GetBonusValueAtLevel(Level));
}

// 특정 레벨의 UI 텍스트
FText UEquipmentUpgradeDataAsset::GetFinalValueTextAtLevel(int32 Level, float BaseValue) const
{
	const int32 Index = GetClampedLevel(Level) - 1;
	if (!LevelDefinitions.IsValidIndex(Index))
	{
		return FText::GetEmpty();
	}

	const FEquipmentUpgradeLevelDefinition& LevelDefinition = LevelDefinitions[Index];
	if (!LevelDefinition.ResultValueTextOverride.IsEmpty())
	{
		return LevelDefinition.ResultValueTextOverride;
	}

	const float FinalValue = GetFinalValueAtLevel(Level, BaseValue);

	FNumberFormattingOptions NumberFormat;
	NumberFormat.MinimumFractionalDigits = 0;
	NumberFormat.MaximumFractionalDigits = 1;

	return ValueUnitText.IsEmpty()
		? FText::AsNumber(FinalValue, &NumberFormat)
		: FText::Format(
			NSLOCTEXT("EquipmentUpgrade", "FinalValueWithUnitFormat", "{0}{1}"),
			FText::AsNumber(FinalValue, &NumberFormat),
			ValueUnitText
		);
}

// 업그레이드 비용
int32 UEquipmentUpgradeDataAsset::GetCostToUpgradeFromLevel(int32 CurrentLevel) const
{
	if (IsMaxLevel(CurrentLevel))
	{
		return 0;
	}

	// 1레벨 -> 2레벨로 갈 때 필요한 비용은 2레벨 데이터(Index: 1)에 있음
	const int32 ClampedCurrentLevel = GetClampedLevel(CurrentLevel);
	if (IsMaxLevel(ClampedCurrentLevel))
	{
		return 0;
	}

	const int32 TargetLevelIndex = ClampedCurrentLevel;
	return LevelDefinitions.IsValidIndex(TargetLevelIndex)
		? FMath::Max(0, LevelDefinitions[TargetLevelIndex].CostToReachLevel)
		: 0;
}

// UI 프리뷰 데이터 빌드
FEquipmentUpgradePreview UEquipmentUpgradeDataAsset::BuildPreview(int32 CurrentLevel, float BaseValue) const
{
	FEquipmentUpgradePreview Preview;
	Preview.UpgradeId = UpgradeId;
	Preview.UpgradeName = UpgradeName;
	Preview.StatLabel = StatLabel;
	Preview.CurrentLevel = GetClampedLevel(CurrentLevel);
	Preview.MaxLevel = GetMaxLevel();
	Preview.bIsMaxLevel = IsMaxLevel(Preview.CurrentLevel);
	Preview.CurrentValueText = GetFinalValueTextAtLevel(Preview.CurrentLevel, BaseValue);
	Preview.NextValueText = Preview.bIsMaxLevel
		? FText::GetEmpty()
		: GetFinalValueTextAtLevel(Preview.CurrentLevel + 1, BaseValue);

	const int32 Cost = GetCostToUpgradeFromLevel(Preview.CurrentLevel);
	Preview.CostText = Preview.bIsMaxLevel
		? FText::GetEmpty()
		: FText::Format(NSLOCTEXT("EquipmentUpgrade", "CostFormat", "{0} CREDITS"), FText::AsNumber(Cost));

	return Preview;
}
