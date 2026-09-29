#include "UI/Title/TitleHostPanel.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/TitleBackend.h"

void UTitleHostPanel::HandleBack()
{
	switch (PanelState)
	{
	case ETitleHostPanelState::ConfirmNew:
		// 초기화 확인은 한 단계만 되돌린다.
		ApplyState(ETitleHostPanelState::Ready);
		NewExpeditionButton->SetKeyboardFocus();
		break;
	case ETitleHostPanelState::Pending:
		// 세션 생성 중에는 닫지 않는다 — 결과가 곧 로비 이동 또는 오류로 온다.
		break;
	default:
		Super::HandleBack();
		break;
	}
}

void UTitleHostPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ContinueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleContinueClicked);
	NewExpeditionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleNewExpeditionClicked);
	ConfirmCancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmCancelClicked);
	ConfirmStartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmStartClicked);
	RetryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRetryClicked);
}

void UTitleHostPanel::NativeDestruct()
{
	if (ITitleBackend* Backend = GetBackend())
	{
		Backend->GetTitleEvents().OnHostComplete.Remove(HostCompleteHandle);
	}

	Super::NativeDestruct();
}

void UTitleHostPanel::OnBackendBound()
{
	HostCompleteHandle = GetBackend()->GetTitleEvents().OnHostComplete.AddUObject(this, &ThisClass::HandleHostComplete);
}

void UTitleHostPanel::OnPanelOpening()
{
	RefreshSaveSummary();

	const ITitleBackend* Backend = GetBackend();
	ApplyState(Backend && Backend->IsHostInFlight() ? ETitleHostPanelState::Pending : ETitleHostPanelState::Ready);
}

UWidget* UTitleHostPanel::GetInitialFocus() const
{
	return bHasResumableProgress ? ContinueButton.Get() : NewExpeditionButton.Get();
}

void UTitleHostPanel::HandleContinueClicked()
{
	RequestHost(ETitleHostMode::Continue);
}

void UTitleHostPanel::HandleNewExpeditionClicked()
{
	// 잃을 진행이 없으면 확인 단계는 의미가 없다.
	if (!bHasResumableProgress)
	{
		RequestHost(ETitleHostMode::NewExpedition);
		return;
	}

	ApplyState(ETitleHostPanelState::ConfirmNew);
	ConfirmCancelButton->SetKeyboardFocus();
}

void UTitleHostPanel::HandleConfirmCancelClicked()
{
	HandleBack();
}

void UTitleHostPanel::HandleConfirmStartClicked()
{
	RequestHost(ETitleHostMode::NewExpedition);
}

void UTitleHostPanel::HandleRetryClicked()
{
	RequestHost(LastRequestedMode);
}

void UTitleHostPanel::HandleHostComplete(bool bWasSuccessful)
{
	if (PanelState != ETitleHostPanelState::Pending)
	{
		return;
	}

	// 성공 시에는 곧바로 로비로 서버 트래블하므로 Pending 표시를 유지한다.
	if (!bWasSuccessful)
	{
		ApplyState(ETitleHostPanelState::Error);
		RetryButton->SetKeyboardFocus();
	}
}

void UTitleHostPanel::RequestHost(ETitleHostMode Mode)
{
	ITitleBackend* Backend = GetBackend();
	if (!Backend || PanelState == ETitleHostPanelState::Pending)
	{
		return;
	}

	LastRequestedMode = Mode;
	ApplyState(ETitleHostPanelState::Pending);
	Backend->RequestHost(Mode);
}

void UTitleHostPanel::RefreshSaveSummary()
{
	const ITitleBackend* Backend = GetBackend();
	bHasResumableProgress = Backend && Backend->HasResumableProgress();

	if (SaveDot)
	{
		SaveDot->SetVisibility(bHasResumableProgress ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (!bHasResumableProgress)
	{
		SaveTitleText->SetText(NoSaveTitle);
		SaveSummaryText->SetText(NoSaveSummary);
		return;
	}

	const FExpeditionProgress Progress = Backend->GetProgressSummary();

	FFormatNamedArguments Args;
	// CurrentRound는 완료한 라운드 수 — 이어하면 그다음 원정부터 시작한다.
	Args.Add(TEXT("Round"), FText::AsNumber(Progress.CurrentRound + 1));
	Args.Add(TEXT("Funds"), FText::AsNumber(Progress.CurrentFunds));

	SaveTitleText->SetText(SavedTitle);
	SaveSummaryText->SetText(FText::Format(SavedSummaryFormat, Args));
}

void UTitleHostPanel::ApplyState(ETitleHostPanelState NewState)
{
	PanelState = NewState;

	const bool bChoicesInteractive = NewState == ETitleHostPanelState::Ready || NewState == ETitleHostPanelState::Error;
	ContinueButton->SetIsEnabled(bChoicesInteractive && bHasResumableProgress);
	NewExpeditionButton->SetIsEnabled(bChoicesInteractive);

	auto ShowIf = [](UWidget* Widget, bool bVisible)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	};
	ShowIf(ConfirmBlock, NewState == ETitleHostPanelState::ConfirmNew);
	ShowIf(PendingBlock, NewState == ETitleHostPanelState::Pending);
	ShowIf(ErrorBlock, NewState == ETitleHostPanelState::Error);
}
