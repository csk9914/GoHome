#include "UI/Title/TitleMenuItemWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UTitleMenuItemWidget::FocusItem()
{
	if (Button)
	{
		Button->SetKeyboardFocus();
	}
}

void UTitleMenuItemWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyStaticStyle();
	RefreshHighlight();
}

void UTitleMenuItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
		Button->OnHovered.AddUniqueDynamic(this, &ThisClass::HandleHovered);
		Button->OnUnhovered.AddUniqueDynamic(this, &ThisClass::HandleUnhovered);
	}
}

void UTitleMenuItemWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveAll(this);
		Button->OnHovered.RemoveAll(this);
		Button->OnUnhovered.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTitleMenuItemWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);

	bFocused = true;
	RefreshHighlight();
}

void UTitleMenuItemWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);

	bFocused = false;
	RefreshHighlight();
}

void UTitleMenuItemWidget::HandleClicked()
{
	OnActivated.Broadcast(this);
}

void UTitleMenuItemWidget::HandleHovered()
{
	bHovered = true;
	// 마우스와 키보드가 서로 다른 항목을 동시에 강조하지 않도록 hover가 포커스를 가져간다.
	FocusItem();
	RefreshHighlight();
}

void UTitleMenuItemWidget::HandleUnhovered()
{
	bHovered = false;
	RefreshHighlight();
}

void UTitleMenuItemWidget::ApplyStaticStyle()
{
	if (Button)
	{
		// 카드형 프레임 없이 행 구분선과 강조 표시만 — 버튼 자체 배경은 그리지 않는다.
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

		FSlateFontInfo Font = LabelText->GetFont();
		Font.Size = bSecondary ? SecondaryLabelFontSize : LabelFontSize;
		Font.TypefaceFontName = bSecondary ? SecondaryLabelTypeface : LabelTypeface;
		Font.LetterSpacing = LabelLetterSpacing;
		LabelText->SetFont(Font);
	}

	const FLinearColor MarkerColor = bSecondary ? SecondaryMarkerColor : PrimaryAccentColor;
	const bool bUseIcon = MarkerIconTexture && MarkerIcon;

	if (MarkerText)
	{
		MarkerText->SetText(Marker);

		FSlateFontInfo MarkerFont = MarkerText->GetFont();
		MarkerFont.Size = MarkerFontSize;
		MarkerFont.TypefaceFontName = MarkerTypeface;
		MarkerText->SetFont(MarkerFont);

		MarkerText->SetColorAndOpacity(FSlateColor(MarkerColor));
		MarkerText->SetVisibility(bUseIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (MarkerIcon)
	{
		if (bUseIcon)
		{
			MarkerIcon->SetBrushFromTexture(MarkerIconTexture);
			MarkerIcon->SetDesiredSizeOverride(MarkerIconSize);
			MarkerIcon->SetColorAndOpacity(MarkerColor);
		}
		MarkerIcon->SetVisibility(bUseIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTitleMenuItemWidget::RefreshHighlight()
{
	const bool bNewHighlighted = bHovered || bFocused;
	const bool bChanged = bNewHighlighted != bHighlighted;
	bHighlighted = bNewHighlighted;

	const FLinearColor Accent = bPrimary ? PrimaryAccentColor : AccentColor;

	if (LabelText)
	{
		const FLinearColor RestColor = bSecondary ? SecondaryLabelColor : LabelColor;
		LabelText->SetColorAndOpacity(FSlateColor(bHighlighted ? HighlightLabelColor : RestColor));
	}

	if (HighlightBar)
	{
		HighlightBar->SetColorAndOpacity(Accent);
		HighlightBar->SetVisibility(bHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (HighlightWash)
	{
		FLinearColor Wash = Accent;
		Wash.A = bPrimary ? 0.4f : 0.12f;
		HighlightWash->SetColorAndOpacity(Wash);
		HighlightWash->SetVisibility(bHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (bChanged && HighlightAnim && !IsDesignTime())
	{
		if (bHighlighted)
		{
			PlayAnimationForward(HighlightAnim);
		}
		else
		{
			PlayAnimationReverse(HighlightAnim);
		}
	}
}
