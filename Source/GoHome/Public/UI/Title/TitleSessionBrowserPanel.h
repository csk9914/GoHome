#pragma once

#include "CoreMinimal.h"
#include "Core/TitleTypes.h"
#include "UI/Title/TitlePanelBase.h"
#include "TitleSessionBrowserPanel.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UTitleSessionRowWidget;

UENUM(BlueprintType)
enum class ETitleBrowserState : uint8
{
	Loading,
	// 결과 0개와 검색 실패는 같은 안내·재검색 동작 — README 상태 계약
	EmptyOrError,
	Ready,
	JoinPending,
	JoinError,
};

/**
 * FIND 패널: 마지막 검색 스냅샷을 표시하고, 열려 있는 동안엔 백엔드 자동 검색을 멈춘다(수동 새로고침만).
 * 선택은 (Generation, SearchIndex)로 기억해 참가 요청이 다른 검색 결과를 가리키지 않게 한다.
 */
UCLASS(Abstract)
class GOHOME_API UTitleSessionBrowserPanel : public UTitlePanelBase
{
	GENERATED_BODY()

public:
	virtual void HandleBack() override;

	UFUNCTION(BlueprintPure, Category = "Title|Browser")
	ETitleBrowserState GetBrowserState() const { return BrowserState; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void OnBackendBound() override;
	virtual void OnPanelOpening() override;
	virtual void OnPanelClosing() override;
	virtual UWidget* GetInitialFocus() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Title|Browser")
	TSubclassOf<UTitleSessionRowWidget> RowClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RowContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ListStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

	// 비활성 버튼 배경 위에서도 읽히도록 라벨 색을 상태별로 바꾼다(.join-button:disabled)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> JoinLabel;

	UPROPERTY(EditAnywhere, Category = "Title|Browser|Style")
	FLinearColor JoinLabelColor = FLinearColor::FromSRGBColor(FColor(0x0D, 0x1C, 0x1C));

	UPROPERTY(EditAnywhere, Category = "Title|Browser|Style")
	FLinearColor JoinLabelDisabledColor = FLinearColor::FromSRGBColor(FColor(0x81, 0x93, 0x8B));

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LoadingBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> EmptyBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FooterText;

	UPROPERTY(EditAnywhere, Category = "Title|Browser|Text")
	FText LoadingStatus = NSLOCTEXT("Title", "BrowserLoading", "탐색 중");

	// {Count} 사용 가능(레퍼런스 문구는 개수를 표시하지 않음)
	UPROPERTY(EditAnywhere, Category = "Title|Browser|Text")
	FText ReadyStatusFormat = NSLOCTEXT("Title", "BrowserReady", "세션 검색 완료");

	UPROPERTY(EditAnywhere, Category = "Title|Browser|Text")
	FText EmptyStatus = NSLOCTEXT("Title", "BrowserEmpty", "검색 완료");

	UPROPERTY(EditAnywhere, Category = "Title|Browser|Text")
	FText FooterHint = NSLOCTEXT("Title", "BrowserFooterHint", "세션을 선택하세요");

	UPROPERTY(EditAnywhere, Category = "Title|Browser|Text")
	FText FooterJoining = NSLOCTEXT("Title", "BrowserFooterJoining", "세션에 참가하는 중…");

	UPROPERTY(EditAnywhere, Category = "Title|Browser|Text")
	FText FooterJoinError = NSLOCTEXT("Title", "BrowserFooterJoinError", "다른 세션을 선택하거나 새로고침하세요");

private:
	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleJoinClicked();

	void HandleSearchUpdated(const FTitleSearchSnapshot& Snapshot);
	void HandleJoinComplete(bool bWasSuccessful);
	void HandleRowClicked(UTitleSessionRowWidget* Row);

	void ApplySnapshot(const FTitleSearchSnapshot& Snapshot);
	void RebuildRows();
	void SelectRow(UTitleSessionRowWidget* Row);
	void ApplyState(ETitleBrowserState NewState);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTitleSessionRowWidget>> Rows;

	FTitleSearchSnapshot DisplayedSnapshot;
	int32 SelectedSearchIndex = INDEX_NONE;
	ETitleBrowserState BrowserState = ETitleBrowserState::Loading;

	FDelegateHandle SearchUpdatedHandle;
	FDelegateHandle JoinCompleteHandle;
};
