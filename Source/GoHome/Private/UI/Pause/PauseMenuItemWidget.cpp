#include "UI/Pause/PauseMenuItemWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Common/HairlineWidget.h"

void UPauseMenuItemWidget::FocusItem()
{
	if (Button)
	{
		Button->SetKeyboardFocus();
	}
}

UWidget* UPauseMenuItemWidget::GetFocusTarget() const
{
	return Button;
}

void UPauseMenuItemWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyStaticStyle();
	RefreshHighlight();
}

void UPauseMenuItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
		Button->OnHovered.AddUniqueDynamic(this, &ThisClass::HandleHovered);
		Button->OnUnhovered.AddUniqueDynamic(this, &ThisClass::HandleUnhovered);
	}
}

void UPauseMenuItemWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveAll(this);
		Button->OnHovered.RemoveAll(this);
		Button->OnUnhovered.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UPauseMenuItemWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);

	bFocused = true;
	RefreshHighlight();
}

void UPauseMenuItemWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);

	bFocused = false;
	RefreshHighlight();
}

void UPauseMenuItemWidget::HandleClicked()
{
	OnActivated.Broadcast(this);
}

void UPauseMenuItemWidget::HandleHovered()
{
	bHovered = true;
	FocusItem();
	RefreshHighlight();
}

void UPauseMenuItemWidget::HandleUnhovered()
{
	bHovered = false;
	RefreshHighlight();
}

void UPauseMenuItemWidget::ApplyStaticStyle()
{
	if (Button)
	{
		// 버튼 자체 배경은 그리지 않는다 — 행 구분선과 강조(워시·막대)만.
		FSlateBrush NoDraw;
		NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;

		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(NoDraw).SetHovered(NoDraw).SetPressed(NoDraw).SetDisabled(NoDraw);
		Style.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
		Button->SetStyle(Style);
	}

	if (LabelText)
	{
		LabelText->SetText(Label);
		LabelText->SetColorAndOpacity(FSlateColor(LabelColor));
	}

	if (GlyphText)
	{
		GlyphText->SetText(Glyph);
		GlyphText->SetColorAndOpacity(FSlateColor(GlyphColor));
	}

	if (MarkerText)
	{
		MarkerText->SetText(FText::FromString(TEXT("›")));
	}

	if (HighlightBar)
	{
		HighlightBar->SetColorAndOpacity(AccentColor);
	}

	if (HighlightWash)
	{
		HighlightWash->SetColorAndOpacity(WashColor);
	}
}

void UPauseMenuItemWidget::RefreshHighlight()
{
	bHighlighted = bHovered || bFocused;

	const ESlateVisibility HighlightVisibility = bHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (HighlightBar)
	{
		HighlightBar->SetVisibility(HighlightVisibility);
	}
	if (HighlightWash)
	{
		HighlightWash->SetVisibility(HighlightVisibility);
	}

	if (MarkerText)
	{
		MarkerText->SetColorAndOpacity(FSlateColor(bHighlighted ? AccentColor : MarkerColor));
	}

	if (BottomLine)
	{
		BottomLine->SetLineColor(bHighlighted ? HighlightLineColor : LineColor);
	}
}
