#include "UI/HUD/OxygenRingWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/OxygenComponent.h"

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

	const FLinearColor Color = Ratio <= CriticalRatio ? CriticalColor : Ratio <= WarningRatio ? WarningColor : NormalColor;
	if (RingMID)
	{
		RingMID->SetScalarParameterValue(PercentParameter, Ratio);
		RingMID->SetVectorParameterValue(ColorParameter, Color);
	}

	ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Ratio * 100.f)));
	ValueText->SetColorAndOpacity(FSlateColor(Color));

	if (BonusText)
	{
		const UOxygenComponent* Oxygen = BoundOxygen.Get();
		const float Bonus = Oxygen ? Oxygen->GetMaxOxygenBonus() : 0.f;
		BonusText->SetText(FText::Format(BonusFormat, FText::AsNumber(FMath::RoundToInt(Bonus))));
		BonusText->SetVisibility(Bonus > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
