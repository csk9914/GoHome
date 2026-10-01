#include "UI/Pause/PauseMenuWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "UI/Pause/PauseConfirmDialog.h"
#include "UI/Pause/PauseMenuItemWidget.h"
#include "UI/Title/TitlePanelBase.h"
#include "UI/Title/TitleToastWidget.h"

void UPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 백엔드가 SetInputMode로 루트에 포커스를 주면 NativeOnFocusReceived가 현재 단계의 대상에게 넘긴다.
	SetIsFocusable(true);
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Backend = Cast<IPauseMenuBackend>(GetOwningPlayer());
	ensureMsgf(Backend.IsValid(), TEXT("UPauseMenuWidget: 오너 PlayerController가 IPauseMenuBackend를 구현하지 않음"));
	if (IPauseMenuBackend* BackendPtr = Backend.Get())
	{
		LeaveFailedHandle = BackendPtr->OnLeaveFailed().AddUObject(this, &ThisClass::HandleLeaveFailed);
	}

	Items.Reset();
	for (UWidget* Child : ItemContainer->GetAllChildren())
	{
		if (UPauseMenuItemWidget* Item = Cast<UPauseMenuItemWidget>(Child))
		{
			Item->OnActivated.AddUObject(this, &ThisClass::HandleItemActivated);
			Items.Add(Item);
		}
	}

	// 시안: 위/아래는 끝에서 반대쪽 끝으로 순환
	if (Items.Num() > 1)
	{
		UWidget* First = Items[0]->GetFocusTarget();
		UWidget* Last = Items.Last()->GetFocusTarget();
		if (First && Last)
		{
			First->SetNavigationRuleExplicit(EUINavigation::Up, Last);
			Last->SetNavigationRuleExplicit(EUINavigation::Down, First);
		}
	}

	SettingsPanel->OnPanelClosed.AddUObject(this, &ThisClass::HandleSettingsClosed);
	SettingsPanel->OnToastRequested.AddUObject(this, &ThisClass::HandleToastRequested);

	ConfirmDialog->OnConfirmed.AddUObject(this, &ThisClass::HandleConfirmed);
	ConfirmDialog->OnCancelled.AddUObject(this, &ThisClass::HandleConfirmCancelled);

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}

	// 첫 포커스는 "게임으로 복귀"
	ReturnFocusItem = Items.Num() > 0 ? Items[0].Get() : nullptr;
	bClosing = false;

	if (OpenAnim)
	{
		PlayAnimationForward(OpenAnim);
	}
}

void UPauseMenuWidget::NativeDestruct()
{
	if (IPauseMenuBackend* BackendPtr = Backend.Get())
	{
		BackendPtr->OnLeaveFailed().Remove(LeaveFailedHandle);
	}

	for (UPauseMenuItemWidget* Item : Items)
	{
		if (Item)
		{
			Item->OnActivated.RemoveAll(this);
		}
	}
	Items.Reset();

	if (SettingsPanel)
	{
		SettingsPanel->OnPanelClosed.RemoveAll(this);
		SettingsPanel->OnToastRequested.RemoveAll(this);
	}
	if (ConfirmDialog)
	{
		ConfirmDialog->OnConfirmed.RemoveAll(this);
		ConfirmDialog->OnCancelled.RemoveAll(this);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const bool bBack = Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
	const bool bStart = Key == EKeys::Gamepad_Special_Right;

	if (bClosing)
	{
		return FReply::Handled();
	}

	if (bBack || bStart)
	{
		// 누르고 있는 동안 여러 단계를 한꺼번에 빠져나가지 않게
		if (InKeyEvent.IsRepeat())
		{
			return FReply::Handled();
		}

		if (ConfirmDialog->IsOpen())
		{
			// 세션 정리 중이면 무시 — 중복 이동 방지
			ConfirmDialog->HandleBack();
		}
		else if (SettingsPanel->IsOpenOrOpening())
		{
			// Start는 메뉴 토글 — 설정 화면에서도 바로 게임으로 복귀(적용 안 한 설정은 버림)
			if (bStart)
			{
				BeginClose();
			}
			else
			{
				SettingsPanel->HandleBack();
			}
		}
		else
		{
			BeginClose();
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UPauseMenuWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	UWidget* Target = GetCurrentFocusTarget();
	if (Target && Target != this)
	{
		return FReply::Handled().SetUserFocus(Target->TakeWidget(), InFocusEvent.GetCause());
	}
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

FReply UPauseMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 빈 배경 클릭으로 포커스가 게임 뷰포트로 새지 않게 현재 단계 대상에 되돌린다.
	if (UWidget* Target = GetCurrentFocusTarget())
	{
		return FReply::Handled().SetUserFocus(Target->TakeWidget(), EFocusCause::Mouse);
	}
	return FReply::Handled();
}

void UPauseMenuWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == CloseAnim && bClosing)
	{
		FinishClose();
	}
}

void UPauseMenuWidget::HandleItemActivated(UPauseMenuItemWidget* Item)
{
	if (!Item || bClosing || ConfirmDialog->IsOpen() || SettingsPanel->IsOpenOrOpening())
	{
		return;
	}

	switch (Item->GetAction())
	{
	case EPauseMenuAction::Resume:
		BeginClose();
		break;
	case EPauseMenuAction::Settings:
		OpenSettings(Item);
		break;
	case EPauseMenuAction::LeaveToTitle:
		OpenConfirm(EPauseLeaveTarget::Title, Item);
		break;
	case EPauseMenuAction::QuitGame:
		OpenConfirm(EPauseLeaveTarget::QuitGame, Item);
		break;
	}
}

void UPauseMenuWidget::OpenSettings(UPauseMenuItemWidget* Source)
{
	ReturnFocusItem = Source;
	MenuView->SetVisibility(ESlateVisibility::Collapsed);
	// 열 때마다 저장된 값으로 다시 채운다(적용 안 한 변경은 닫으면 버림) — UTitleSettingsPanel::OnPanelOpening
	SettingsPanel->OpenPanel();
}

void UPauseMenuWidget::HandleSettingsClosed(UTitlePanelBase* /*Panel*/)
{
	MenuView->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FocusItem(ReturnFocusItem.Get());
}

void UPauseMenuWidget::HandleToastRequested(const FText& Message)
{
	if (Toast)
	{
		Toast->ShowMessage(Message);
	}
}

void UPauseMenuWidget::OpenConfirm(EPauseLeaveTarget Target, UPauseMenuItemWidget* Source)
{
	ReturnFocusItem = Source;
	ConfirmTarget = Target;

	const bool bHost = IsHost();
	FPauseConfirmContent Content;
	if (Target == EPauseLeaveTarget::Title)
	{
		Content.Eyebrow = TitleEyebrow;
		Content.Heading = TitleHeading;
		Content.Body = bHost ? TitleBodyHost : TitleBodyParticipant;
		Content.ConfirmLabel = TitleConfirmLabel;
	}
	else
	{
		Content.Eyebrow = QuitEyebrow;
		Content.Heading = QuitHeading;
		Content.Body = bHost ? QuitBodyHost : QuitBodyParticipant;
		Content.ConfirmLabel = QuitConfirmLabel;
		Content.bWarning = true;
	}

	// 팝업이 떠 있는 동안 뒤 메뉴는 마우스·포커스 이동 대상에서 빠진다
	MenuWindow->SetVisibility(ESlateVisibility::HitTestInvisible);
	ConfirmDialog->Open(Content);
}

void UPauseMenuWidget::CloseConfirm()
{
	ConfirmTarget.Reset();
	ConfirmDialog->Close();
	MenuWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FocusItem(ReturnFocusItem.Get());
}

void UPauseMenuWidget::HandleConfirmed()
{
	IPauseMenuBackend* BackendPtr = Backend.Get();
	if (!BackendPtr || !ConfirmTarget.IsSet() || BackendPtr->IsLeaveInFlight())
	{
		return;
	}

	// 요청 전에 잠근다 — 정리할 세션이 없으면 백엔드가 이 호출 안에서 곧바로 이동을 시작한다.
	ConfirmDialog->SetBusy(IsHost() ? BusyHost : BusyParticipant);
	BackendPtr->RequestLeave(ConfirmTarget.GetValue());
}

void UPauseMenuWidget::HandleConfirmCancelled()
{
	CloseConfirm();
}

void UPauseMenuWidget::HandleLeaveFailed(EPauseLeaveTarget Target, const FText& Reason)
{
	if (ConfirmDialog->IsOpen() && ConfirmTarget == Target)
	{
		// 팝업을 유지한 채 오류 + "다시 시도". 취소하면 메뉴로 돌아간다.
		ConfirmDialog->ShowError(Reason);
	}
}

void UPauseMenuWidget::HandleCloseClicked()
{
	if (!ConfirmDialog->IsOpen())
	{
		BeginClose();
	}
}

void UPauseMenuWidget::BeginClose()
{
	if (bClosing)
	{
		return;
	}

	// 닫힘이 시작되는 즉시 UI 입력만 잠근다. 게임 입력 복원은 닫힘이 끝난 뒤 백엔드가 한다.
	bClosing = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (OpenAnim && IsAnimationPlaying(OpenAnim))
	{
		StopAnimation(OpenAnim);
	}

	if (CloseAnim)
	{
		PlayAnimationForward(CloseAnim);
	}
	else
	{
		FinishClose();
	}
}

void UPauseMenuWidget::FinishClose()
{
	if (IPauseMenuBackend* BackendPtr = Backend.Get())
	{
		BackendPtr->RequestResume();
	}
}

void UPauseMenuWidget::FocusItem(UPauseMenuItemWidget* Item)
{
	if (!Item && Items.Num() > 0)
	{
		Item = Items[0];
	}
	if (Item)
	{
		Item->FocusItem();
	}
}

UWidget* UPauseMenuWidget::GetCurrentFocusTarget() const
{
	if (ConfirmDialog && ConfirmDialog->IsOpen())
	{
		return ConfirmDialog;
	}
	if (SettingsPanel && SettingsPanel->IsOpenOrOpening())
	{
		return SettingsPanel;
	}
	// 방향키/hover로 옮겨 둔 행이 있으면 그 행(배경 클릭 직후에도 강조 상태가 남아 있다)
	for (UPauseMenuItemWidget* Item : Items)
	{
		if (Item && Item->IsHighlighted())
		{
			return Item->GetFocusTarget();
		}
	}
	if (UPauseMenuItemWidget* Item = ReturnFocusItem.Get())
	{
		return Item->GetFocusTarget();
	}
	return Items.Num() > 0 ? Items[0]->GetFocusTarget() : nullptr;
}

bool UPauseMenuWidget::IsHost() const
{
	const IPauseMenuBackend* BackendPtr = Backend.Get();
	return !BackendPtr || BackendPtr->GetSessionRole() == EPauseSessionRole::Host;
}
