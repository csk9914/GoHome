#include "UI/Title/TitleScreenWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Core/TitleBackend.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/Title/TitleMenuItemWidget.h"
#include "UI/Title/TitleMenuWidget.h"
#include "UI/Title/TitlePanelBase.h"
#include "UI/Title/TitleToastWidget.h"

void UTitleScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 메뉴 항목이 아직 포커스를 갖지 않은 진입 상태에서도 방향키/Escape를 받기 위해 화면 자체가 포커스를 받는다.
	SetIsFocusable(true);
}

void UTitleScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Backend = Cast<ITitleBackend>(GetOwningPlayer());
	ensureMsgf(Backend.IsValid(), TEXT("UTitleScreenWidget: 오너 PlayerController가 ITitleBackend를 구현하지 않음"));

	if (ITitleBackend* BackendPtr = Backend.Get())
	{
		HostCompleteHandle = BackendPtr->GetTitleEvents().OnHostComplete.AddUObject(this, &ThisClass::HandleHostComplete);
		JoinCompleteHandle = BackendPtr->GetTitleEvents().OnJoinComplete.AddUObject(this, &ThisClass::HandleJoinComplete);
	}

	if (MainMenu)
	{
		MainMenu->OnItemActivated.AddUObject(this, &ThisClass::HandleMenuItemActivated);
	}

	for (UTitlePanelBase* Panel : { HostPanel.Get(), SessionBrowserPanel.Get(), SettingsPanel.Get() })
	{
		if (Panel)
		{
			Panel->BindBackend(Backend.Get());
			Panel->OnPanelClosed.AddUObject(this, &ThisClass::HandlePanelClosed);
		}
	}

	if (Shade)
	{
		Shade->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleShadeClicked);
	}
	SetShadeVisible(false);

	if (IntroAnim)
	{
		PlayAnimationForward(IntroAnim);
	}
}

void UTitleScreenWidget::NativeDestruct()
{
	if (ITitleBackend* BackendPtr = Backend.Get())
	{
		BackendPtr->GetTitleEvents().OnHostComplete.Remove(HostCompleteHandle);
		BackendPtr->GetTitleEvents().OnJoinComplete.Remove(JoinCompleteHandle);
	}

	if (MainMenu)
	{
		MainMenu->OnItemActivated.RemoveAll(this);
	}

	for (UTitlePanelBase* Panel : { HostPanel.Get(), SessionBrowserPanel.Get(), SettingsPanel.Get() })
	{
		if (Panel)
		{
			Panel->OnPanelClosed.RemoveAll(this);
		}
	}

	if (Shade)
	{
		Shade->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UTitleScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (UTitlePanelBase* Panel = ActivePanel.Get())
		{
			Panel->HandleBack();
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	// 진입 시 선택 없음 — 아직 아무 항목도 강조되지 않았을 때 첫 방향 입력으로 메뉴에 들어온다.
	if (!ActivePanel.IsValid() && MainMenu && !MainMenu->HasHighlightedItem())
	{
		if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
		{
			MainMenu->FocusFirstItem();
			return FReply::Handled();
		}
		if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
		{
			MainMenu->FocusLastItem();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UTitleScreenWidget::HandleMenuItemActivated(UTitleMenuItemWidget* Item)
{
	if (!Item || ActivePanel.IsValid())
	{
		return;
	}

	if (Item->GetAction() == ETitleMenuAction::Quit)
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		return;
	}

	OpenPanel(FindPanel(Item->GetAction()), Item);
}

void UTitleScreenWidget::HandlePanelClosed(UTitlePanelBase* Panel)
{
	if (ActivePanel.Get() != Panel)
	{
		return;
	}

	ActivePanel.Reset();
	SetShadeVisible(false);

	if (UTitleMenuItemWidget* Item = ReturnFocusItem.Get())
	{
		Item->FocusItem();
	}
	else
	{
		SetKeyboardFocus();
	}
	ReturnFocusItem.Reset();
}

void UTitleScreenWidget::HandleShadeClicked()
{
	if (UTitlePanelBase* Panel = ActivePanel.Get())
	{
		Panel->RequestClose();
	}
}

UTitlePanelBase* UTitleScreenWidget::FindPanel(ETitleMenuAction Action) const
{
	switch (Action)
	{
	case ETitleMenuAction::Host:     return HostPanel;
	case ETitleMenuAction::Find:     return SessionBrowserPanel;
	case ETitleMenuAction::Settings: return SettingsPanel;
	default:                         return nullptr;
	}
}

void UTitleScreenWidget::OpenPanel(UTitlePanelBase* Panel, UTitleMenuItemWidget* Source)
{
	if (!Panel)
	{
		return;
	}

	ActivePanel = Panel;
	ReturnFocusItem = Source;
	SetShadeVisible(true);
	Panel->OpenPanel();
}

void UTitleScreenWidget::SetShadeVisible(bool bVisible)
{
	if (Shade)
	{
		Shade->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UTitleScreenWidget::HandleHostComplete(bool bWasSuccessful)
{
	ShowToast(bWasSuccessful ? HostSuccessMessage : HostFailureMessage);
}

void UTitleScreenWidget::HandleJoinComplete(bool bWasSuccessful)
{
	// 참가 실패 안내는 세션 목록 패널이 직접 표시한다.
	if (bWasSuccessful)
	{
		ShowToast(JoinSuccessMessage);
	}
}

void UTitleScreenWidget::ShowToast(const FText& Message)
{
	if (Toast)
	{
		Toast->ShowMessage(Message);
	}
}
