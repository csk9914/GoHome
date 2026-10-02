#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OxygenRingWidget.generated.h"

class UEquipmentUpgradeDataAsset;
class UImage;
class UMaterialInstanceDynamic;
class UOxygenComponent;
class UTextBlock;

/**
 * 하단 중앙 O₂ 링(Reference Pack: Docs/Dev/UI/ingame-hud B안). HP 링과 같은 링 아트(MI_HP_Ring의 HPPercent/HPColor)를 청록으로 쓴다.
 * 단계: 정상 청록 → 25% 이하 주황 → 10% 이하 빨강+1초 깜빡임.
 * 강화(산소 용량) 레벨에 따라 링 전체 색이 바뀐다: 기본 청록 → Lv2 초록 → Lv3 파랑 → Lv4 보라(TierColors). 경고 색이 우선.
 * (Pack: Docs/Dev/UI/ingame-hud/o2-ring-tiers.html B안 — 구간 분할 A안은 사용자 결정으로 기각)
 * 소유 폰의 UOxygenComponent에 스스로 바인딩하고, 폰이 바뀌면 다시 바인딩, 폰이 없으면(관전) 접힌다.
 */
UCLASS(Abstract)
class GOHOME_API UOxygenRingWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 머티리얼(MI_HP_Ring 계열)을 쓰는 채움 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RingFill;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BonusText;

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring")
	FName PercentParameter = TEXT("HPPercent");

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring")
	FName ColorParameter = TEXT("HPColor");

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring", meta = (ClampMin = "0", ClampMax = "1"))
	float WarningRatio = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring", meta = (ClampMin = "0", ClampMax = "1"))
	float CriticalRatio = 0.10f;

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring", meta = (ClampMin = "0.1"))
	float CriticalBlinkPeriod = 1.f;

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring", meta = (ClampMin = "0", ClampMax = "1"))
	float CriticalBlinkMinOpacity = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Color")
	FLinearColor NormalColor = FLinearColor::FromSRGBColor(FColor(0x19, 0xE3, 0xEE));

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Color")
	FLinearColor WarningColor = FLinearColor::FromSRGBColor(FColor(0xFF, 0xB5, 0x47));

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Color")
	FLinearColor CriticalColor = FLinearColor::FromSRGBColor(FColor(0xFF, 0x4D, 0x4D));

	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Text")
	FText BonusFormat = NSLOCTEXT("OxygenRing", "Bonus", "+{0}");

	// 레벨별 누적 보너스로 현재 강화 등급을 역산(DA_OxygenUpgrade). 없으면 보너스가 있으면 1등급으로.
	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Tiers")
	TObjectPtr<UEquipmentUpgradeDataAsset> UpgradeData;

	// 강화 등급 색(Lv2, Lv3, Lv4 …). 모자라면 마지막 색 반복
	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Tiers")
	TArray<FLinearColor> TierColors = {
		FLinearColor::FromSRGBColor(FColor(0x4C, 0xE3, 0x6B)),
		FLinearColor::FromSRGBColor(FColor(0x4F, 0x7D, 0xFF)),
		FLinearColor::FromSRGBColor(FColor(0xC0, 0x64, 0xFF)) };

	// 강화 등급 색 밝기 배율(링 머티리얼이 트랙 텍스처와 곱해 어두워지는 만큼 보정)
	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Tiers", meta = (ClampMin = "0"))
	float TierBrightness = 2.f;

private:
	UFUNCTION()
	void HandleOxygenChanged(float CurrentOxygen, float MaxOxygen);

	void BindToPawn(APawn* Pawn);
	void Unbind();

	// 보너스 → 현재 강화 등급(0 = 강화 없음)
	int32 ComputeUpgradeTier(float Bonus) const;
	FLinearColor GetTierColor(int32 UpgradeTierIndex) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	int32 UpgradeTier = 0;

	TWeakObjectPtr<UOxygenComponent> BoundOxygen;
	TWeakObjectPtr<APawn> BoundPawn;

	float Ratio = 1.f;
	float BlinkElapsed = 0.f;
};
