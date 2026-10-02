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
 * 강화(산소 용량)는 예리도 게이지식 구간 — 강화 레벨마다 늘어난 용량이 링 끝에 다음 등급 색(TierColors)으로 붙고 위 등급부터 소모된다.
 * 같은 링 머티리얼(0→N% 채움)을 런타임에 여러 겹 복제해 아래(위 등급)→위(기본) 순으로 겹쳐 구간을 만든다(Pack: Docs/Dev/UI/ingame-hud/o2-ring-tiers.html A안).
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

	// 레벨별 누적 보너스로 등급 경계를 계산(DA_OxygenUpgrade). 없으면 보너스 전체를 한 등급으로.
	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Tiers")
	TObjectPtr<UEquipmentUpgradeDataAsset> UpgradeData;

	// 강화 등급 색(Lv2, Lv3, Lv4 …). 모자라면 마지막 색 반복
	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Tiers")
	TArray<FLinearColor> TierColors = {
		FLinearColor::FromSRGBColor(FColor(0x4C, 0xE3, 0x6B)),
		FLinearColor::FromSRGBColor(FColor(0x4F, 0x7D, 0xFF)),
		FLinearColor::FromSRGBColor(FColor(0xC0, 0x64, 0xFF)) };

	// 비어 있는 강화 구간 = 트랙색과 등급색 사이(이 비율만큼 등급색)
	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Tiers", meta = (ClampMin = "0", ClampMax = "1"))
	float EmptyTierStrength = 0.3f;

	// 링 트랙 텍스처(T_HP_Ring_Track) 평균색 — 강화 구간이 있을 때 기본 구간의 빈 자리를 덮는 데 쓴다
	UPROPERTY(EditAnywhere, Category = "Oxygen Ring|Tiers")
	FLinearColor TrackColor = FLinearColor::FromSRGBColor(FColor(93, 100, 108, 209));

private:
	UFUNCTION()
	void HandleOxygenChanged(float CurrentOxygen, float MaxOxygen);

	void BindToPawn(APawn* Pawn);
	void Unbind();

	// 보너스 → 구간 경계(0..1) 재계산. 구간 수가 바뀌면 레이어 재생성
	void UpdateTiers(float BaseMax, float Bonus);
	void RebuildLayers(int32 UpgradeTierCount);
	UImage* AddLayerBelowFill();
	void ApplyLayers();
	FLinearColor GetTierColor(int32 UpgradeTierIndex) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	// RingFill의 원본 머티리얼(MID 만들기 전) — 레이어마다 자기 MID를 갖도록 이걸로 브러시를 만든다
	UPROPERTY(Transient)
	TObjectPtr<UObject> RingMaterialTemplate;

	// 아래→위: FaintLayers(위 등급부터) · BaseCover · FillLayers(위 등급부터) · RingFill(기본)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> FaintLayers;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BaseCover;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> FillLayers;

	// 0, 기본/최대, (기본+Lv2)/최대, …, 1
	TArray<float> Bounds;

	TWeakObjectPtr<UOxygenComponent> BoundOxygen;
	TWeakObjectPtr<APawn> BoundPawn;

	float Ratio = 1.f;
	float BlinkElapsed = 0.f;
};
