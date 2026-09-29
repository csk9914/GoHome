#include "UI/Title/TitlePanelBase.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Core/TitleBackend.h"

void UTitlePanelBase::BindBackend(ITitleBackend* InBackend)
{
	Backend = InBackend;
	if (InBackend)
	{
		OnBackendBound();
	}
}

void UTitlePanelBase::OpenPanel()
{
	if (IsOpenOrOpening())
	{
		return;
	}

	if (CloseAnim && IsAnimationPlaying(CloseAnim))
	{
		StopAnimation(CloseAnim);
	}

	PanelState = EPanelState::Opening;
	SetVisibility(ESlateVisibility::Visible);
	OnPanelOpening();

	if (OpenAnim)
	{
		PlayAnimationForward(OpenAnim);
	}
	else
	{
		FinishOpen();
	}
}

void UTitlePanelBase::RequestClose()
{
	if (PanelState == EPanelState::Closed || PanelState == EPanelState::Closing)
	{
		return;
	}

	if (OpenAnim && IsAnimationPlaying(OpenAnim))
	{
		StopAnimation(OpenAnim);
	}

	// 닫기 요청 즉시 입력 잠금 — 닫히는 중인 패널의 버튼이 눌리지 않게.
	PanelState = EPanelState::Closing;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	OnPanelClosing();

	if (CloseAnim)
	{
		PlayAnimationForward(CloseAnim);
	}
	else
	{
		FinishClose();
	}
}

void UTitlePanelBase::HandleBack()
{
	RequestClose();
}

void UTitlePanelBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Collapsed);

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void UTitlePanelBase::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTitlePanelBase::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == OpenAnim && PanelState == EPanelState::Opening)
	{
		FinishOpen();
	}
	else if (Animation == CloseAnim && PanelState == EPanelState::Closing)
	{
		FinishClose();
	}
}

void UTitlePanelBase::HandleCloseClicked()
{
	RequestClose();
}

void UTitlePanelBase::FinishOpen()
{
	PanelState = EPanelState::Open;

	if (UWidget* FocusTarget = GetInitialFocus())
	{
		FocusTarget->SetKeyboardFocus();
	}
	else
	{
		SetKeyboardFocus();
	}
}

void UTitlePanelBase::FinishClose()
{
	PanelState = EPanelState::Closed;
	SetVisibility(ESlateVisibility::Collapsed);
	OnPanelClosed.Broadcast(this);
}
