#include "UI/HUD/OxygenRingWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/OxygenComponent.h"
#include "Upgrade/EquipmentUpgradeDataAsset.h"

void UOxygenRingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RingMID = RingFill->GetDynamicMaterial();
	BindToPawn(GetOwningPlayerPawn());
}

void UOxygenRingWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UOxygenRingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 폰 교체(사망→관전, 로비 재생성) 감지 — 위젯은 뷰라 폰 스코프 데이터를 따라간다
	APawn* Pawn = GetOwningPlayerPawn();
	if (Pawn != BoundPawn.Get())
	{
		BindToPawn(Pawn);
	}

	float Opacity = 1.f;
	if (BoundOxygen.IsValid() && Ratio <= CriticalRatio)
	{
		BlinkElapsed = FMath::Fmod(BlinkElapsed + InDeltaTime, CriticalBlinkPeriod);
		// motion.md: step 1 ↔ 0.35
		Opacity = BlinkElapsed < CriticalBlinkPeriod * 0.5f ? 1.f : CriticalBlinkMinOpacity;
	}
	else
	{
		BlinkElapsed = 0.f;
	}
	RingFill->SetRenderOpacity(Opacity);
	ValueText->SetRenderOpacity(Opacity);
}

void UOxygenRingWidget::BindToPawn(APawn* Pawn)
{
	Unbind();
	BoundPawn = Pawn;

	UOxygenComponent* Oxygen = Pawn ? Pawn->FindComponentByClass<UOxygenComponent>() : nullptr;
	SetVisibility(Oxygen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Oxygen)
	{
		return;
	}

	BoundOxygen = Oxygen;
	Oxygen->OnOxygenChanged.AddUniqueDynamic(this, &UOxygenRingWidget::HandleOxygenChanged);
	HandleOxygenChanged(Oxygen->GetOxygen(), Oxygen->GetMaxOxygen());
}

void UOxygenRingWidget::Unbind()
{
	if (UOxygenComponent* Oxygen = BoundOxygen.Get())
	{
		Oxygen->OnOxygenChanged.RemoveDynamic(this, &UOxygenRingWidget::HandleOxygenChanged);
	}
	BoundOxygen.Reset();
	BoundPawn.Reset();
}

void UOxygenRingWidget::HandleOxygenChanged(float CurrentOxygen, float MaxOxygen)
{
	Ratio = MaxOxygen > 0.f ? FMath::Clamp(CurrentOxygen / MaxOxygen, 0.f, 1.f) : 0.f;

	const UOxygenComponent* Oxygen = BoundOxygen.Get();
	const float Bonus = Oxygen ? Oxygen->GetMaxOxygenBonus() : 0.f;
	UpgradeTier = ComputeUpgradeTier(Bonus);

	// 링 색: 경고 단계 > 강화 등급 > 기본 청록
	FLinearColor RingColor = NormalColor;
	if (Ratio <= CriticalRatio)
	{
		RingColor = CriticalColor;
	}
	else if (Ratio <= WarningRatio)
	{
		RingColor = WarningColor;
	}
	else if (UpgradeTier > 0)
	{
		RingColor = GetTierColor(UpgradeTier - 1) * TierBrightness;
		RingColor.A = 1.f;
	}

	if (RingMID)
	{
		RingMID->SetScalarParameterValue(PercentParameter, Ratio);
		RingMID->SetVectorParameterValue(ColorParameter, RingColor);
	}

	// 숫자는 경고 단계만 반영(등급색은 링으로 충분, 숫자는 청록 유지)
	const FLinearColor TextColor = Ratio <= CriticalRatio ? CriticalColor : Ratio <= WarningRatio ? WarningColor : NormalColor;
	ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Ratio * 100.f)));
	ValueText->SetColorAndOpacity(FSlateColor(TextColor));

	if (BonusText)
	{
		BonusText->SetText(FText::Format(BonusFormat, FText::AsNumber(FMath::RoundToInt(Bonus))));
		BonusText->SetVisibility(Bonus > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		BonusText->SetColorAndOpacity(FSlateColor(GetTierColor(FMath::Max(0, UpgradeTier - 1))));
	}
}

int32 UOxygenRingWidget::ComputeUpgradeTier(float Bonus) const
{
	if (Bonus <= KINDA_SMALL_NUMBER)
	{
		return 0;
	}
	if (!UpgradeData)
	{
		return 1;
	}

	// 복제된 보너스 값으로 레벨 역산: 보너스가 증가하는 레벨마다 한 등급
	int32 Tier = 0;
	float Previous = 0.f;
	for (int32 Level = 2; Level <= UpgradeData->GetMaxLevel(); ++Level)
	{
		const float LevelBonus = UpgradeData->GetBonusValueAtLevel(Level);
		if (LevelBonus <= Previous + KINDA_SMALL_NUMBER || LevelBonus > Bonus + KINDA_SMALL_NUMBER)
		{
			continue;
		}
		++Tier;
		Previous = LevelBonus;
	}
	return FMath::Max(Tier, 1);
}

FLinearColor UOxygenRingWidget::GetTierColor(int32 UpgradeTierIndex) const
{
	if (TierColors.IsEmpty())
	{
		return NormalColor;
	}
	return TierColors[FMath::Clamp(UpgradeTierIndex, 0, TierColors.Num() - 1)];
}
