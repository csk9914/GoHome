#include "UI/Title/TitleSessionRowWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UTitleSessionRowWidget::SetListing(const FTitleSessionListing& InListing, int32 DisplayNumber)
{
	Listing = InListing;
	NumberText->SetText(FText::FromString(FString::Printf(TEXT("%02d"), DisplayNumber)));
	HostNameText->SetText(Listing.HostName);
	SetSelected(false);
}

void UTitleSessionRowWidget::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	RefreshVisual();
}

void UTitleSessionRowWidget::SetInteractive(bool bInteractive)
{
	RowButton->SetIsEnabled(bInteractive);
}

UWidget* UTitleSessionRowWidget::GetFocusTarget() const
{
	return RowButton;
}

void UTitleSessionRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FSlateBrush NoDraw;
	NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;
	FButtonStyle Style = RowButton->GetStyle();
	Style.SetNormal(NoDraw).SetHovered(NoDraw).SetPressed(NoDraw).SetDisabled(NoDraw);
	Style.SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
	RowButton->SetStyle(Style);

	RowButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
	RowButton->OnHovered.AddUniqueDynamic(this, &ThisClass::HandleHovered);
	RowButton->OnUnhovered.AddUniqueDynamic(this, &ThisClass::HandleUnhovered);

	RefreshVisual();
}

void UTitleSessionRowWidget::HandleClicked()
{
	OnRowClicked.Broadcast(this);
}

void UTitleSessionRowWidget::HandleHovered()
{
	bHovered = true;
	RowButton->SetKeyboardFocus();
	RefreshVisual();
}

void UTitleSessionRowWidget::HandleUnhovered()
{
	bHovered = false;
	RefreshVisual();
}

void UTitleSessionRowWidget::RefreshVisual()
{
	const ESlateVisibility Emphasis = (bSelected || bHovered) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;

	if (SelectedWash)
	{
		SelectedWash->SetVisibility(Emphasis);
	}

	if (ArrowText)
	{
		ArrowText->SetVisibility(Emphasis);
	}
}
