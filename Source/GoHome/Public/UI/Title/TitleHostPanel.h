#pragma once

#include "CoreMinimal.h"
#include "Core/TitleTypes.h"
#include "UI/Title/TitlePanelBase.h"
#include "TitleHostPanel.generated.h"

class UButton;
class UTextBlock;

UENUM(BlueprintType)
enum class ETitleHostPanelState : uint8
{
	Ready,
	ConfirmNew,
	Pending,
	Error,
};

/**
 * HOST 패널: 저장 진행 요약 + 이어하기 / 새 원정(확인 후).
 * 버튼은 요청만 하고, 상태 전이는 백엔드의 OnHostComplete 결과로 일어난다.
 */
UCLASS(Abstract)
class GOHOME_API UTitleHostPanel : public UTitlePanelBase
{
	GENERATED_BODY()

public:
	virtual void HandleBack() override;

	UFUNCTION(BlueprintPure, Category = "Title|Host")
	ETitleHostPanelState GetPanelState() const { return PanelState; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void OnBackendBound() override;
	virtual void OnPanelOpening() override;
	virtual UWidget* GetInitialFocus() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NewExpeditionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SaveTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SaveSummaryText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ConfirmBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmCancelButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmStartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> PendingBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ErrorBlock;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RetryButton;

	UPROPERTY(EditAnywhere, Category = "Title|Host|Text")
	FText SavedTitle = NSLOCTEXT("Title", "HostSavedTitle", "저장된 원정");

	UPROPERTY(EditAnywhere, Category = "Title|Host|Text")
	FText NoSaveTitle = NSLOCTEXT("Title", "HostNoSaveTitle", "저장된 원정 없음");

	// {Round} = 다음에 시작할 원정 번호, {Funds} = 보유 자금
	UPROPERTY(EditAnywhere, Category = "Title|Host|Text")
	FText SavedSummaryFormat = NSLOCTEXT("Title", "HostSavedSummary", "{Round}번째 원정 · 자금 {Funds}");

	UPROPERTY(EditAnywhere, Category = "Title|Host|Text")
	FText NoSaveSummary = NSLOCTEXT("Title", "HostNoSaveSummary", "새 원정으로 시작합니다");

private:
	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleNewExpeditionClicked();

	UFUNCTION()
	void HandleConfirmCancelClicked();

	UFUNCTION()
	void HandleConfirmStartClicked();

	UFUNCTION()
	void HandleRetryClicked();

	void HandleHostComplete(bool bWasSuccessful);

	void RequestHost(ETitleHostMode Mode);
	void RefreshSaveSummary();
	void ApplyState(ETitleHostPanelState NewState);

	ETitleHostPanelState PanelState = ETitleHostPanelState::Ready;
	ETitleHostMode LastRequestedMode = ETitleHostMode::Continue;
	bool bHasResumableProgress = false;
	FDelegateHandle HostCompleteHandle;
};
