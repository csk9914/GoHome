#include "UI/HUD/OxygenRingWidget.h"

#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/OxygenComponent.h"
#include "Upgrade/EquipmentUpgradeDataAsset.h"

namespace
{
	void SetRing(UImage* Layer, FName PercentParameter, FName ColorParameter, float Percent, const FLinearColor& Color, FName StartParameter = NAME_None, float Start = 0.f)
	{
		if (!Layer)
		{
			return;
		}
		if (UMaterialInstanceDynamic* MID = Layer->GetDynamicMaterial())
		{
			MID->SetScalarParameterValue(PercentParameter, Percent);
			MID->SetVectorParameterValue(ColorParameter, Color);
			if (!StartParameter.IsNone())
			{
				MID->SetScalarParameterValue(StartParameter, Start);
			}
		}
	}
}

void UOxygenRingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!RingMaterialTemplate)
	{
		UObject* Resource = RingFill->GetBrush().GetResourceObject();
		const UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(Resource);
		UObject* Original = ExistingMID ? ExistingMID->Parent.Get() : Resource;
		RingMaterialTemplate = SegmentMaterial ? static_cast<UObject*>(SegmentMaterial.Get()) : Original;
	}
	RingMID = RingFill->GetDynamicMaterial();
	Bounds = { 0.f, 1.f };
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
	UpdateTiers(Oxygen ? Oxygen->GetBaseMaxOxygen() : MaxOxygen, Bonus);
	ApplyLayers();

	const bool bWarning = Ratio <= WarningRatio;
	const FLinearColor Color = Ratio <= CriticalRatio ? CriticalColor : bWarning ? WarningColor : NormalColor;
	ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Ratio * 100.f)));
	ValueText->SetColorAndOpacity(FSlateColor(Color));

	if (BonusText)
	{
		BonusText->SetText(FText::Format(BonusFormat, FText::AsNumber(FMath::RoundToInt(Bonus))));
		BonusText->SetVisibility(Bonus > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// "+N"은 현재 최고 등급 색
		BonusText->SetColorAndOpacity(FSlateColor(GetTierColor(FMath::Max(0, Bounds.Num() - 3))));
	}
}

void UOxygenRingWidget::UpdateTiers(float BaseMax, float Bonus)
{
	// 누적 용량 경계: 기본, 기본+Lv2 보너스, …, 기본+현재 보너스
	TArray<float> Capacities;
	Capacities.Add(BaseMax);
	if (Bonus > KINDA_SMALL_NUMBER)
	{
		if (UpgradeData)
		{
			float Previous = 0.f;
			for (int32 Level = 2; Level <= UpgradeData->GetMaxLevel(); ++Level)
			{
				const float LevelBonus = UpgradeData->GetBonusValueAtLevel(Level);
				if (LevelBonus <= Previous + KINDA_SMALL_NUMBER || LevelBonus > Bonus + KINDA_SMALL_NUMBER)
				{
					continue;
				}
				Capacities.Add(BaseMax + LevelBonus);
				Previous = LevelBonus;
			}
		}
		// 데이터에 없는 보너스(테스트·임시 버프 등) 잔여분은 마지막 등급으로
		if (Capacities.Last() < BaseMax + Bonus - KINDA_SMALL_NUMBER)
		{
			Capacities.Add(BaseMax + Bonus);
		}
	}

	const float Total = FMath::Max(Capacities.Last(), KINDA_SMALL_NUMBER);
	TArray<float> NewBounds;
	NewBounds.Add(0.f);
	for (float Capacity : Capacities)
	{
		NewBounds.Add(Capacity / Total);
	}

	const int32 UpgradeTierCount = NewBounds.Num() - 2;
	if (UpgradeTierCount != FillLayers.Num())
	{
		RebuildLayers(UpgradeTierCount);
	}
	Bounds = MoveTemp(NewBounds);
}

UImage* UOxygenRingWidget::AddLayerBelowFill()
{
	UPanelWidget* Parent = RingFill->GetParent();
	if (!Parent)
	{
		return nullptr;
	}

	UImage* Layer = NewObject<UImage>(this);
	FSlateBrush LayerBrush = RingFill->GetBrush();
	LayerBrush.SetResourceObject(RingMaterialTemplate);
	Layer->SetBrush(LayerBrush);
	Layer->SetVisibility(ESlateVisibility::HitTestInvisible);

	UPanelSlot* NewSlot = Parent->InsertChildAt(Parent->GetChildIndex(RingFill), Layer);
	const UOverlaySlot* FillSlot = Cast<UOverlaySlot>(RingFill->Slot);
	if (UOverlaySlot* LayerSlot = Cast<UOverlaySlot>(NewSlot); LayerSlot && FillSlot)
	{
		LayerSlot->SetPadding(FillSlot->GetPadding());
		LayerSlot->SetHorizontalAlignment(FillSlot->GetHorizontalAlignment());
		LayerSlot->SetVerticalAlignment(FillSlot->GetVerticalAlignment());
	}
	return Layer;
}

void UOxygenRingWidget::RebuildLayers(int32 UpgradeTierCount)
{
	for (UImage* Layer : FaintLayers)
	{
		if (Layer)
		{
			Layer->RemoveFromParent();
		}
	}
	for (UImage* Layer : FillLayers)
	{
		if (Layer)
		{
			Layer->RemoveFromParent();
		}
	}
	FaintLayers.Reset();
	FillLayers.Reset();

	if (UpgradeTierCount <= 0)
	{
		return;
	}

	// RingFill 바로 아래에 끼워 넣는다(구간 머티리얼이라 서로 안 겹침)
	for (int32 Tier = UpgradeTierCount - 1; Tier >= 0; --Tier)
	{
		FaintLayers.Insert(AddLayerBelowFill(), 0);
	}
	for (int32 Tier = UpgradeTierCount - 1; Tier >= 0; --Tier)
	{
		FillLayers.Insert(AddLayerBelowFill(), 0);
	}

	// UOverlay::InsertChildAt은 UMG 목록에만 끼우고 Slate 오버레이엔 맨 위로 붙인다 → 그리기 순서가 뒤집힘.
	// 목록 순서대로 다시 붙여 Slate 순서를 맞춘다(슬롯 설정 보존).
	if (UPanelWidget* Parent = RingFill->GetParent())
	{
		struct FChildSlot
		{
			UWidget* Widget = nullptr;
			FMargin Padding;
			TEnumAsByte<EHorizontalAlignment> HAlign = HAlign_Fill;
			TEnumAsByte<EVerticalAlignment> VAlign = VAlign_Fill;
		};
		TArray<FChildSlot> Children;
		for (UWidget* Child : Parent->GetAllChildren())
		{
			FChildSlot& Entry = Children.AddDefaulted_GetRef();
			Entry.Widget = Child;
			if (const UOverlaySlot* OldSlot = Cast<UOverlaySlot>(Child->Slot))
			{
				Entry.Padding = OldSlot->GetPadding();
				Entry.HAlign = OldSlot->GetHorizontalAlignment();
				Entry.VAlign = OldSlot->GetVerticalAlignment();
			}
		}
		Parent->ClearChildren();
		for (const FChildSlot& Entry : Children)
		{
			if (UOverlaySlot* NewSlot = Cast<UOverlaySlot>(Parent->AddChild(Entry.Widget)))
			{
				NewSlot->SetPadding(Entry.Padding);
				NewSlot->SetHorizontalAlignment(Entry.HAlign);
				NewSlot->SetVerticalAlignment(Entry.VAlign);
			}
		}
	}
}

void UOxygenRingWidget::ApplyLayers()
{
	const bool bWarning = Ratio <= WarningRatio;
	const FLinearColor WarnColor = Ratio <= CriticalRatio ? CriticalColor : WarningColor;
	const float BaseEnd = Bounds.IsValidIndex(1) ? Bounds[1] : 1.f;

	// 기본 구간(맨 위): 경고면 남은 링 전체를 경고색으로
	SetRing(RingFill, PercentParameter, ColorParameter, bWarning ? Ratio : FMath::Min(Ratio, BaseEnd), bWarning ? WarnColor : NormalColor);

	for (int32 Tier = 0; Tier < FillLayers.Num(); ++Tier)
	{
		const float TierStart = Bounds.IsValidIndex(Tier + 1) ? Bounds[Tier + 1] : 1.f;
		const float TierEnd = Bounds.IsValidIndex(Tier + 2) ? Bounds[Tier + 2] : 1.f;
		FLinearColor TierColor = GetTierColor(Tier) * TierBrightness;
		TierColor.A = 1.f;
		FLinearColor EmptyColor = TierColor * EmptyTierStrength;
		EmptyColor.A = 1.f;
		// 채움: [시작, min(남은 양, 끝)] — 남은 양이 시작 전이면 폭 0
		const float FillEnd = bWarning ? TierStart : FMath::Clamp(Ratio, TierStart, TierEnd);
		SetRing(FillLayers[Tier], PercentParameter, ColorParameter, FillEnd, TierColor, StartParameter, TierStart);
		// 폭 0이어도 경계 페더(smoothstep)로 실선이 남으니 숨긴다
		FillLayers[Tier]->SetRenderOpacity(FillEnd > TierStart + 0.001f ? 1.f : 0.f);
		SetRing(FaintLayers[Tier], PercentParameter, ColorParameter, TierEnd, EmptyColor, StartParameter, TierStart);
	}
}

FLinearColor UOxygenRingWidget::GetTierColor(int32 UpgradeTierIndex) const
{
	if (TierColors.IsEmpty())
	{
		return NormalColor;
	}
	return TierColors[FMath::Clamp(UpgradeTierIndex, 0, TierColors.Num() - 1)];
}
