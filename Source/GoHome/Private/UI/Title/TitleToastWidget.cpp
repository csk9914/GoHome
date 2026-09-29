#include "UI/Title/TitleToastWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UTitleToastWidget::ShowMessage(const FText& Message)
{
	MessageText->SetText(Message);

	if (HideAnim && IsAnimationPlaying(HideAnim))
	{
		StopAnimation(HideAnim);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (ShowAnim)
	{
		PlayAnimationForward(ShowAnim);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HoldTimer, this, &ThisClass::Hide, HoldSeconds, false);
	}
}

void UTitleToastWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Collapsed);
}

void UTitleToastWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimer);
	}

	Super::NativeDestruct();
}

void UTitleToastWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == HideAnim)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTitleToastWidget::Hide()
{
	if (HideAnim)
	{
		PlayAnimationForward(HideAnim);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
