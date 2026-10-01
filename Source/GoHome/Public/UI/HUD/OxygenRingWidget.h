#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OxygenRingWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UOxygenComponent;
class UTextBlock;

/**
 * 하단 중앙 O₂ 링(Reference Pack: Docs/Dev/UI/ingame-hud B안). HP 링과 같은 링 아트(MI_HP_Ring의 HPPercent/HPColor)를 청록으로 쓴다.
 * 단계: 정상 청록 → 25% 이하 주황 → 10% 이하 빨강+1초 깜빡임. 강화 보너스(GetMaxOxygenBonus)는 금색 "+N" 라벨.
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

private:
	UFUNCTION()
	void HandleOxygenChanged(float CurrentOxygen, float MaxOxygen);

	void BindToPawn(APawn* Pawn);
	void Unbind();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	TWeakObjectPtr<UOxygenComponent> BoundOxygen;
	TWeakObjectPtr<APawn> BoundPawn;

	float Ratio = 1.f;
	float BlinkElapsed = 0.f;
};
