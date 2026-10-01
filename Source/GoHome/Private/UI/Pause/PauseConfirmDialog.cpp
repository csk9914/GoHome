#include "UI/Pause/PauseConfirmDialog.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPauseConfirmDialog::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Collapsed);
	CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancelClicked);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmClicked);
}

void UPauseConfirmDialog::NativeDestruct()
{
	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveAll(this);
	}
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UPauseConfirmDialog::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	// 정리 중엔 팝업 자체가 포커스를 쥔다(버튼 비활성). 그 외엔 취소 버튼으로 넘긴다.
	if (!bBusy && CancelButton)
	{
		return FReply::Handled().SetUserFocus(CancelButton->TakeWidget(), InFocusEvent.GetCause());
	}
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

void UPauseConfirmDialog::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bOpen)
	{
		RefreshFocusVisuals();
	}
}

void UPauseConfirmDialog::RefreshFocusVisuals()
{
	if (CancelLabel)
	{
		const bool bCancelFocused = CancelButton->HasKeyboardFocus() || CancelButton->IsHovered();
		CancelLabel->SetColorAndOpacity(FSlateColor(bCancelFocused ? CancelFocusColor : CancelColor));
	}

	const FLinearColor Base = CurrentContent.bWarning ? WarningColor : ConfirmColor;
	const bool bConfirmFocused = ConfirmButton->HasKeyboardFocus();
	ConfirmButton->SetBackgroundColor(bConfirmFocused ? Base * ConfirmFocusBrighten : Base);
}

void UPauseConfirmDialog::Open(const FPauseConfirmContent& Content)
{
	CurrentContent = Content;
	bOpen = true;
	bBusy = false;

	EyebrowText->SetText(Content.Eyebrow);
	EyebrowText->SetColorAndOpacity(FSlateColor(Content.bWarning ? WarningEyebrowColor : EyebrowColor));
	HeadingText->SetText(Content.Heading);
	BodyText->SetText(Content.Body);
	ConfirmLabel->SetText(Content.ConfirmLabel);
	ConfirmButton->SetBackgroundColor(Content.bWarning ? WarningColor : ConfirmColor);
	StatusText->SetText(FText::GetEmpty());
	StatusText->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsEnabled(true);

	// scrim이 배경 메뉴의 마우스 입력을 막는다
	SetVisibility(ESlateVisibility::Visible);
	if (OpenAnim)
	{
		PlayAnimationForward(OpenAnim);
	}
	FocusDefault();
}

void UPauseConfirmDialog::Close()
{
	if (OpenAnim && IsAnimationPlaying(OpenAnim))
	{
		StopAnimation(OpenAnim);
	}

	bOpen = false;
	bBusy = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPauseConfirmDialog::SetBusy(const FText& StatusMessage)
{
	bBusy = true;
	SetButtonsEnabled(false);

	StatusText->SetText(StatusMessage);
	StatusText->SetColorAndOpacity(FSlateColor(StatusColor));
	StatusText->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 버튼이 비활성화되면 포커스를 잃으므로 팝업 자체가 포커스를 쥐고 Escape를 계속 받는다.
	SetKeyboardFocus();
}

void UPauseConfirmDialog::ShowError(const FText& Message)
{
	bBusy = false;
	SetButtonsEnabled(true);

	StatusText->SetText(Message);
	StatusText->SetColorAndOpacity(FSlateColor(ErrorColor));
	StatusText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ConfirmLabel->SetText(RetryLabel);

	// 재시도가 가장 가능성 높은 다음 행동
	ConfirmButton->SetKeyboardFocus();
}

void UPauseConfirmDialog::HandleBack()
{
	if (!bBusy)
	{
		HandleCancelClicked();
	}
}

void UPauseConfirmDialog::FocusDefault()
{
	// 시안: 팝업이 열리면 취소에 포커스 — 실수로 Enter를 눌러도 세션을 떠나지 않게.
	CancelButton->SetKeyboardFocus();
}

void UPauseConfirmDialog::HandleCancelClicked()
{
	if (bBusy)
	{
		return;
	}
	OnCancelled.Broadcast();
}

void UPauseConfirmDialog::HandleConfirmClicked()
{
	if (bBusy)
	{
		return;
	}
	OnConfirmed.Broadcast();
}

void UPauseConfirmDialog::SetButtonsEnabled(bool bEnabled)
{
	CancelButton->SetIsEnabled(bEnabled);
	ConfirmButton->SetIsEnabled(bEnabled);
}
