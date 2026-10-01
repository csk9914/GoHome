#include "UI/HUD/ExpeditionProgressHUDWidget.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/GoHomeGameState.h"
#include "Engine/World.h"

namespace
{
	void SetShown(UWidget* Widget, bool bShown)
	{
		if (Widget)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	float EaseOutCubic(float T)
	{
		const float U = 1.f - FMath::Clamp(T, 0.f, 1.f);
		return 1.f - U * U * U;
	}
}

void UExpeditionProgressHUDWidget::FCountUp::SetTarget(float NewTarget, float InCountUpSeconds)
{
	if (!bInitialized || InCountUpSeconds <= 0.f)
	{
		// 첫 값은 카운트업 없이 바로(맵 진입 시 0 → 1,000 으로 굴러가지 않게)
		From = To = Displayed = NewTarget;
		Elapsed = InCountUpSeconds;
		FlashElapsed = TNumericLimits<float>::Max();
		bInitialized = true;
		return;
	}

	if (FMath::IsNearlyEqual(NewTarget, To))
	{
		return;
	}

	// 카운트업 중 새 갱신이 오면 현재 표시값에서 새 목표로 다시 시작(motion.md)
	From = Displayed;
	To = NewTarget;
	Elapsed = 0.f;
	FlashElapsed = 0.f;
}

bool UExpeditionProgressHUDWidget::FCountUp::Tick(float DeltaTime, float InCountUpSeconds, float InFlashSeconds)
{
	const bool bWasAnimating = Elapsed < InCountUpSeconds || FlashElapsed < InFlashSeconds;
	Elapsed += DeltaTime;
	FlashElapsed = FlashElapsed < TNumericLimits<float>::Max() ? FlashElapsed + DeltaTime : FlashElapsed;
	Displayed = InCountUpSeconds > 0.f ? FMath::Lerp(From, To, EaseOutCubic(Elapsed / InCountUpSeconds)) : To;
	return bWasAnimating;
}

void UExpeditionProgressHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UWorld* World = GetWorld();
	AGoHomeGameState* GameState = World ? World->GetGameState<AGoHomeGameState>() : nullptr;
	if (GameState)
	{
		BoundGameState = GameState;
		GameState->OnExpeditionProgressChanged.AddUniqueDynamic(this, &ThisClass::HandleProgressChanged);
	}

	BuildTimeSegments();

	AppearElapsed = 0.f;
	SetRenderOpacity(AppearSeconds > 0.f ? 0.f : 1.f);

	// 바인딩 직후 현재 값을 한 번 당겨온다(서버가 이미 세팅을 끝낸 뒤 생성되는 경우)
	Refresh();

	LastDisplayedSeconds = INDEX_NONE;
	LastSegmentsOn = INDEX_NONE;
	CurrentStageColor = HudCyanColor;
	TickTimeLimit(0.f);
}

void UExpeditionProgressHUDWidget::NativeDestruct()
{
	if (AGoHomeGameState* GameState = BoundGameState.Get())
	{
		GameState->OnExpeditionProgressChanged.RemoveAll(this);
	}
	BoundGameState.Reset();

	Super::NativeDestruct();
}

void UExpeditionProgressHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickValues(InDeltaTime);

	// 남은 시간은 복제된 마감 서버시각에서 매 순간 계산되는 값이라 델리게이트가 아니라 틱으로 읽는다
	TickTimeLimit(InDeltaTime);
}

void UExpeditionProgressHUDWidget::HandleProgressChanged()
{
	Refresh();
}

void UExpeditionProgressHUDWidget::BuildTimeSegments()
{
	if (!TimeSegments || SegmentImages.Num() == TimeSegmentCount)
	{
		return;
	}

	TimeSegments->ClearChildren();
	SegmentImages.Reset();

	for (int32 Index = 0; Index < TimeSegmentCount; ++Index)
	{
		UImage* Segment = NewObject<UImage>(this);
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(2.f, 2.f, 2.f, 2.f);
		Brush.ImageSize = TimeSegmentSize;
		Segment->SetBrush(Brush);

		if (UHorizontalBoxSlot* SegmentSlot = TimeSegments->AddChildToHorizontalBox(Segment))
		{
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < TimeSegmentCount ? TimeSegmentGap : 0.f, 0.f));
			SegmentSlot->SetVerticalAlignment(VAlign_Center);
		}
		SegmentImages.Add(Segment);
	}
}

void UExpeditionProgressHUDWidget::Refresh()
{
	const AGoHomeGameState* GameState = BoundGameState.Get();

	// 접속 직후 복제 전(전체 라운드 0)이면 숫자 대신 "—"
	bPending = !GameState || GameState->GetFinalRound() <= 0;

	if (bPending)
	{
		Txt_Round->SetText(FText::Format(RoundFormat, FFormatNamedArguments{ { TEXT("Current"), PendingText }, { TEXT("Final"), PendingText } }));
	}
	else
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Current"), FText::AsNumber(GameState->GetCurrentRound() + 1));
		Args.Add(TEXT("Final"), FText::AsNumber(GameState->GetFinalRound()));
		Txt_Round->SetText(FText::Format(RoundFormat, Args));
	}

	// 할당량(탐사맵만) — 없으면 로비로 본다
	int32 Delivered = 0;
	int32 Quota = 0;
	bHasQuota = GameState && GameState->GetMapQuotaProgress(Delivered, Quota);
	MapQuota = Quota;
	SetShown(QuotaSection, bHasQuota);
	SetShown(Txt_LobbyTag, !bHasQuota);
	if (bHasQuota)
	{
		DeliveredValue.SetTarget(Delivered, CountUpSeconds);
		Txt_QuotaMax->SetText(FText::Format(NSLOCTEXT("ProgressHUD", "OfMax", "/ {0}"), FText::AsNumber(MapQuota)));
	}

	// 다음 관문
	bHasCheckPoint = GameState && GameState->GetNextCheckPointRound() > 0;
	CheckPointQuota = GameState ? GameState->GetNextCheckPointQuota() : 0;
	if (GameState)
	{
		FundsValue.SetTarget(GameState->GetCurrentFunds(), CountUpSeconds);
	}

	if (bHasCheckPoint)
	{
		Txt_CheckPointLabel->SetText(FText::Format(CheckPointLabelFormat, FFormatNamedArguments{ { TEXT("Round"), FText::AsNumber(GameState->GetNextCheckPointRound()) } }));
		Txt_CheckPointTarget->SetText(FText::Format(NSLOCTEXT("ProgressHUD", "OfMax", "/ {0}"), FText::AsNumber(CheckPointQuota)));
	}
	else
	{
		Txt_CheckPointLabel->SetText(AllClearedLabelText);
		Txt_Funds->SetText(AllClearedText);
	}
	SetShown(Txt_CheckPointTarget, bHasCheckPoint);
	SetShown(Bar_CheckPoint, bHasCheckPoint);

	ApplyDisplayedValues();
}

void UExpeditionProgressHUDWidget::TickValues(float DeltaTime)
{
	if (AppearElapsed < AppearSeconds)
	{
		AppearElapsed += DeltaTime;
		SetRenderOpacity(FMath::Clamp(AppearElapsed / AppearSeconds, 0.f, 1.f));
	}

	const bool bFundsAnimating = FundsValue.Tick(DeltaTime, CountUpSeconds, ValueFlashSeconds);
	const bool bDeliveredAnimating = DeliveredValue.Tick(DeltaTime, CountUpSeconds, ValueFlashSeconds);
	if (bFundsAnimating || bDeliveredAnimating)
	{
		ApplyDisplayedValues();
	}
}

void UExpeditionProgressHUDWidget::ApplyDisplayedValues()
{
	auto FlashColor = [this](const FCountUp& Value)
	{
		const float T = ValueFlashSeconds > 0.f ? FMath::Clamp(Value.FlashElapsed / ValueFlashSeconds, 0.f, 1.f) : 1.f;
		return FSlateColor(FMath::Lerp(ValueFlashColor, InkColor, EaseOutCubic(T)));
	};

	if (bHasQuota)
	{
		const int32 Shown = FMath::RoundToInt(DeliveredValue.Displayed);
		const bool bMet = MapQuota > 0 && DeliveredValue.To >= MapQuota;
		Txt_QuotaValue->SetText(bPending ? PendingText : FText::AsNumber(Shown));
		Txt_QuotaValue->SetColorAndOpacity(FlashColor(DeliveredValue));
		Txt_QuotaPct->SetText(FText::Format(NSLOCTEXT("ProgressHUD", "Pct", "{0}%"), FText::AsNumber(MapQuota > 0 ? FMath::FloorToInt(DeliveredValue.To * 100.f / MapQuota) : 0)));
		SetShown(Txt_QuotaPct, !bMet && !bPending);
		SetShown(Chip_QuotaMet, bMet && !bPending);
		Bar_Quota->SetPercent(bPending || MapQuota <= 0 ? 0.f : FMath::Clamp(Shown / static_cast<float>(MapQuota), 0.f, 1.f));
		Bar_Quota->SetFillColorAndOpacity(bMet ? OkColor : HudCyanColor);
	}

	if (bHasCheckPoint)
	{
		const int32 Shown = FMath::RoundToInt(FundsValue.Displayed);
		const int32 Short = FMath::Max(0, CheckPointQuota - FMath::RoundToInt(FundsValue.To));
		Txt_Funds->SetText(bPending ? PendingText : FText::AsNumber(Shown));
		Txt_Funds->SetColorAndOpacity(FlashColor(FundsValue));
		Txt_Short->SetText(FText::Format(ShortFormat, FFormatNamedArguments{ { TEXT("Short"), FText::AsNumber(Short) } }));
		SetShown(Chip_Short, Short > 0 && !bPending);
		SetShown(Chip_CheckPointMet, Short == 0 && !bPending);
		Bar_CheckPoint->SetPercent(bPending || CheckPointQuota <= 0 ? 0.f : FMath::Clamp(Shown / static_cast<float>(CheckPointQuota), 0.f, 1.f));
		Bar_CheckPoint->SetFillColorAndOpacity(Short > 0 ? WarningColor : OkColor);
	}
	else
	{
		Txt_Funds->SetColorAndOpacity(FSlateColor(InkColor));
		SetShown(Chip_Short, false);
		SetShown(Chip_CheckPointMet, false);
	}
}

FLinearColor UExpeditionProgressHUDWidget::StageColor(ETimeLimitStage Stage) const
{
	switch (Stage)
	{
	case ETimeLimitStage::Warning:
		return WarningColor;
	case ETimeLimitStage::Critical:
	case ETimeLimitStage::TimeUp:
		return CriticalColor;
	default:
		return HudCyanColor;
	}
}

void UExpeditionProgressHUDWidget::TickTimeLimit(float DeltaTime)
{
	const AGoHomeGameState* GameState = BoundGameState.Get();
	float RemainingSeconds = 0.f;
	float TotalSeconds = 0.f;
	const bool bHasTimeLimit = GameState && GameState->GetTimeLimitProgress(RemainingSeconds, TotalSeconds);

	if (bHasTimeLimit != bTimeSectionVisible)
	{
		bTimeSectionVisible = bHasTimeLimit;
		SetShown(TimeSection, bHasTimeLimit);
		LastDisplayedSeconds = INDEX_NONE;
		LastSegmentsOn = INDEX_NONE;
	}

	if (!bHasTimeLimit)
	{
		SetShown(Chip_TimeState, false);
		return;
	}

	const float Ratio = TotalSeconds > 0.f ? FMath::Clamp(RemainingSeconds / TotalSeconds, 0.f, 1.f) : 0.f;
	const ETimeLimitStage Stage = RemainingSeconds <= 0.f ? ETimeLimitStage::TimeUp
		: Ratio <= CriticalRatio ? ETimeLimitStage::Critical
		: Ratio <= WarningRatio ? ETimeLimitStage::Warning
		: ETimeLimitStage::Normal;

	// 단계 색은 StageColorBlendSeconds 동안 선형으로 넘어간다
	const FLinearColor TargetColor = StageColor(Stage);
	if (StageColorBlendSeconds > 0.f && DeltaTime > 0.f)
	{
		const float Step = DeltaTime / StageColorBlendSeconds;
		CurrentStageColor = FMath::Lerp(CurrentStageColor, TargetColor, FMath::Clamp(Step, 0.f, 1.f));
		if (CurrentStageColor.Equals(TargetColor, 0.01f))
		{
			CurrentStageColor = TargetColor;
		}
	}
	else
	{
		CurrentStageColor = TargetColor;
	}

	// 표시 초(올림 — 0:00은 실제로 끝났을 때만)
	const int32 DisplayedSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	if (DisplayedSeconds != LastDisplayedSeconds || Stage != LastStage)
	{
		LastDisplayedSeconds = DisplayedSeconds;
		LastStage = Stage;

		Txt_Time->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), DisplayedSeconds / 60, DisplayedSeconds % 60)));

		const int32 TotalShown = FMath::Max(0, FMath::CeilToInt(TotalSeconds));
		Txt_TimeTotal->SetText(FText::Format(TimeTotalFormat, FFormatNamedArguments{ { TEXT("Total"), FText::FromString(FString::Printf(TEXT("%d:%02d"), TotalShown / 60, TotalShown % 60)) } }));

		const bool bShowChip = Stage != ETimeLimitStage::Normal;
		SetShown(Chip_TimeState, bShowChip);
		if (bShowChip)
		{
			Txt_TimeState->SetText(Stage == ETimeLimitStage::Warning ? WarningStateText
				: Stage == ETimeLimitStage::Critical ? CriticalStateText
				: TimeUpStateText);
		}
	}

	// 색(보간 중이면 매 틱)
	const FSlateColor TimeTextColor(Stage == ETimeLimitStage::Normal ? InkColor : CurrentStageColor);
	Txt_Time->SetColorAndOpacity(TimeTextColor);
	Txt_TimeState->SetColorAndOpacity(FSlateColor(CurrentStageColor));
	Chip_TimeState->SetBrushColor(CurrentStageColor);

	const int32 SegmentsOn = FMath::CeilToInt(Ratio * SegmentImages.Num());
	for (int32 Index = 0; Index < SegmentImages.Num(); ++Index)
	{
		FLinearColor SegmentColor = CurrentStageColor;
		SegmentColor.A = Index < SegmentsOn ? 1.f : TimeSegmentOffOpacity;
		SegmentImages[Index]->SetColorAndOpacity(SegmentColor);
	}
	LastSegmentsOn = SegmentsOn;

	// 위험 단계에선 숫자만 주기적으로 깜빡인다(코사인 — 켜진 상태로 시작). 종료(0:00)에선 멈춘다.
	float Opacity = 1.f;
	if (Stage == ETimeLimitStage::Critical && CriticalBlinkPeriod > 0.f)
	{
		const UWorld* World = GetWorld();
		const float Time = World ? World->GetTimeSeconds() : 0.f;
		const float Wave = 0.5f + 0.5f * FMath::Cos(2.f * PI * Time / CriticalBlinkPeriod);
		Opacity = FMath::Lerp(CriticalBlinkMinOpacity, 1.f, Wave);
	}
	Txt_Time->SetRenderOpacity(Opacity);
}
