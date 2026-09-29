#include "UI/Title/TitleSessionBrowserPanel.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Core/TitleBackend.h"
#include "UI/Title/TitleSessionRowWidget.h"

void UTitleSessionBrowserPanel::HandleBack()
{
	// 참가 요청 중에는 닫지 않는다 — 결과가 곧 트래블 또는 오류로 온다.
	if (BrowserState == ETitleBrowserState::JoinPending)
	{
		return;
	}

	Super::HandleBack();
}

void UTitleSessionBrowserPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRefreshClicked);
	JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleJoinClicked);
}

void UTitleSessionBrowserPanel::NativeDestruct()
{
	if (ITitleBackend* Backend = GetBackend())
	{
		Backend->GetTitleEvents().OnSearchUpdated.Remove(SearchUpdatedHandle);
		Backend->GetTitleEvents().OnJoinComplete.Remove(JoinCompleteHandle);
		Backend->SetAutoRefreshPaused(false);
	}

	Super::NativeDestruct();
}

void UTitleSessionBrowserPanel::OnBackendBound()
{
	FTitleBackendEvents& Events = GetBackend()->GetTitleEvents();
	SearchUpdatedHandle = Events.OnSearchUpdated.AddUObject(this, &ThisClass::HandleSearchUpdated);
	JoinCompleteHandle = Events.OnJoinComplete.AddUObject(this, &ThisClass::HandleJoinComplete);
}

void UTitleSessionBrowserPanel::OnPanelOpening()
{
	ITitleBackend* Backend = GetBackend();
	if (!Backend)
	{
		return;
	}

	Backend->SetAutoRefreshPaused(true);
	ApplySnapshot(Backend->GetSearchSnapshot());

	if (Backend->GetSearchSnapshot().Status == ETitleSearchStatus::Idle)
	{
		Backend->RequestRefresh();
	}
}

void UTitleSessionBrowserPanel::OnPanelClosing()
{
	if (ITitleBackend* Backend = GetBackend())
	{
		Backend->SetAutoRefreshPaused(false);
	}
}

UWidget* UTitleSessionBrowserPanel::GetInitialFocus() const
{
	if (BrowserState == ETitleBrowserState::Ready && Rows.Num() > 0)
	{
		return Rows[0]->GetFocusTarget();
	}
	return RefreshButton;
}

void UTitleSessionBrowserPanel::HandleRefreshClicked()
{
	if (ITitleBackend* Backend = GetBackend())
	{
		Backend->RequestRefresh();
	}
}

void UTitleSessionBrowserPanel::HandleJoinClicked()
{
	ITitleBackend* Backend = GetBackend();
	if (!Backend || SelectedSearchIndex == INDEX_NONE)
	{
		return;
	}

	if (Backend->RequestJoin(DisplayedSnapshot.Generation, SelectedSearchIndex))
	{
		ApplyState(ETitleBrowserState::JoinPending);
		return;
	}

	// 보이는 목록이 이미 낡음 — 최신 결과로 바꾸고 다시 선택하게 한다.
	ApplySnapshot(Backend->GetSearchSnapshot());
	ApplyState(ETitleBrowserState::JoinError);
}

void UTitleSessionBrowserPanel::HandleSearchUpdated(const FTitleSearchSnapshot& Snapshot)
{
	// 닫혀 있을 땐 표시하지 않는다(열 때 백엔드의 최신 스냅샷을 읽는다). 참가 중엔 목록을 바꾸지 않는다.
	if (!IsOpenOrOpening() || BrowserState == ETitleBrowserState::JoinPending)
	{
		return;
	}

	ApplySnapshot(Snapshot);
}

void UTitleSessionBrowserPanel::HandleJoinComplete(bool bWasSuccessful)
{
	if (BrowserState != ETitleBrowserState::JoinPending)
	{
		return;
	}

	// 성공 시에는 세션 주소로 트래블하므로 Pending 표시를 유지한다.
	if (!bWasSuccessful)
	{
		ApplyState(ETitleBrowserState::JoinError);
	}
}

void UTitleSessionBrowserPanel::HandleRowClicked(UTitleSessionRowWidget* Row)
{
	if (BrowserState == ETitleBrowserState::Ready || BrowserState == ETitleBrowserState::JoinError)
	{
		SelectRow(Row);
	}
}

void UTitleSessionBrowserPanel::ApplySnapshot(const FTitleSearchSnapshot& Snapshot)
{
	const bool bNewResults = Snapshot.Generation != DisplayedSnapshot.Generation;
	DisplayedSnapshot = Snapshot;

	switch (Snapshot.Status)
	{
	case ETitleSearchStatus::Idle:
	case ETitleSearchStatus::Searching:
		ApplyState(ETitleBrowserState::Loading);
		break;
	case ETitleSearchStatus::Ready:
		if (bNewResults || Rows.Num() != Snapshot.Listings.Num())
		{
			RebuildRows();
		}
		ApplyState(ETitleBrowserState::Ready);
		break;
	case ETitleSearchStatus::Empty:
	case ETitleSearchStatus::Failed:
		RebuildRows();
		ApplyState(ETitleBrowserState::EmptyOrError);
		break;
	}
}

void UTitleSessionBrowserPanel::RebuildRows()
{
	for (UTitleSessionRowWidget* Row : Rows)
	{
		Row->OnRowClicked.RemoveAll(this);
	}
	Rows.Reset();
	RowContainer->ClearChildren();
	SelectedSearchIndex = INDEX_NONE;

	if (!RowClass)
	{
		return;
	}

	for (int32 Index = 0; Index < DisplayedSnapshot.Listings.Num(); ++Index)
	{
		UTitleSessionRowWidget* Row = CreateWidget<UTitleSessionRowWidget>(this, RowClass);
		Row->SetListing(DisplayedSnapshot.Listings[Index], Index + 1);
		Row->OnRowClicked.AddUObject(this, &ThisClass::HandleRowClicked);
		RowContainer->AddChild(Row);
		Rows.Add(Row);
	}
}

void UTitleSessionBrowserPanel::SelectRow(UTitleSessionRowWidget* Row)
{
	SelectedSearchIndex = Row ? Row->GetListing().SearchIndex : INDEX_NONE;

	for (UTitleSessionRowWidget* Each : Rows)
	{
		Each->SetSelected(Each == Row);
	}

	ApplyState(BrowserState == ETitleBrowserState::JoinError ? ETitleBrowserState::Ready : BrowserState);
}

void UTitleSessionBrowserPanel::ApplyState(ETitleBrowserState NewState)
{
	BrowserState = NewState;

	const bool bShowList = NewState == ETitleBrowserState::Ready || NewState == ETitleBrowserState::JoinPending || NewState == ETitleBrowserState::JoinError;
	RowContainer->SetVisibility(bShowList ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	LoadingBlock->SetVisibility(NewState == ETitleBrowserState::Loading ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	EmptyBlock->SetVisibility(NewState == ETitleBrowserState::EmptyOrError ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	const bool bLocked = NewState == ETitleBrowserState::Loading || NewState == ETitleBrowserState::JoinPending;
	RefreshButton->SetIsEnabled(!bLocked);
	JoinButton->SetIsEnabled(!bLocked && bShowList && SelectedSearchIndex != INDEX_NONE);

	for (UTitleSessionRowWidget* Row : Rows)
	{
		Row->SetInteractive(NewState != ETitleBrowserState::JoinPending);
	}

	switch (NewState)
	{
	case ETitleBrowserState::Loading:
		ListStatusText->SetText(LoadingStatus);
		break;
	case ETitleBrowserState::EmptyOrError:
		ListStatusText->SetText(EmptyStatus);
		break;
	default:
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Count"), FText::AsNumber(Rows.Num()));
		ListStatusText->SetText(FText::Format(ReadyStatusFormat, Args));
		break;
	}
	}

	switch (NewState)
	{
	case ETitleBrowserState::JoinPending:
		FooterText->SetText(FooterJoining);
		break;
	case ETitleBrowserState::JoinError:
		FooterText->SetText(FooterJoinError);
		break;
	default:
		FooterText->SetText(FooterHint);
		break;
	}
}
