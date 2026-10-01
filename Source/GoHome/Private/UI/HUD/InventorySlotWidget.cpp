#include "UI/HUD/InventorySlotWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"

void UInventorySlotWidget::SetSlot(int32 InSlotIndex, const AItemActorBase* Item, bool bInActive)
{
	KeyText->SetText(FText::AsNumber(InSlotIndex + 1));

	const UItemDataAsset* Data = Item ? Item->ItemData.Get() : nullptr;
	UTexture2D* Icon = Data ? Data->Icon.Get() : nullptr;
	ItemIcon->SetBrushFromTexture(Icon);
	ItemIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (FallbackText)
	{
		const bool bFallback = Data && !Icon;
		FallbackText->SetText(bFallback ? FText::FromString(Data->DisplayName.ToString().Left(2)) : FText::GetEmpty());
		FallbackText->SetVisibility(bFallback ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Item)
	{
		FNumberFormattingOptions Format;
		Format.MaximumFractionalDigits = 1;
		WeightText->SetText(FText::Format(NSLOCTEXT("Hotbar", "Weight", "{0}kg"), FText::AsNumber(Item->GetTotalWeight(), &Format)));
		WeightText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		WeightText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (EmptyMark)
	{
		EmptyMark->SetVisibility(Item ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	bActive = bInActive;
	if (!bHasAppliedState)
	{
		// 첫 표시는 모션 없이 바로
		ActiveAlpha = bActive ? 1.f : 0.f;
		bHasAppliedState = true;
	}
	ApplyBorder(ActiveAlpha);
}

void UInventorySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float Target = bActive ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(ActiveAlpha, Target))
	{
		return;
	}
	const float Step = ActiveSeconds > 0.f ? InDeltaTime / ActiveSeconds : 1.f;
	ActiveAlpha = FMath::Clamp(ActiveAlpha + (Target > ActiveAlpha ? Step : -Step), 0.f, 1.f);
	ApplyBorder(ActiveAlpha);
}

void UInventorySlotWidget::ApplyBorder(float Alpha)
{
	// ease-out
	const float Eased = 1.f - FMath::Square(1.f - Alpha);

	FSlateBrush Brush = SlotBorder->Background;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	Brush.OutlineSettings.CornerRadii = FVector4(2.f, 2.f, 2.f, 2.f);
	Brush.OutlineSettings.Width = FMath::Lerp(IdleBorderWidth, ActiveBorderWidth, Eased);
	Brush.OutlineSettings.Color = FSlateColor(FMath::Lerp(IdleBorderColor, ActiveBorderColor, Eased));
	SlotBorder->SetBrush(Brush);

	KeyText->SetColorAndOpacity(FSlateColor(FMath::Lerp(IdleKeyColor, ActiveBorderColor, Eased)));
	SetRenderTranslation(FVector2D(0.f, -ActiveLift * Eased));
}
