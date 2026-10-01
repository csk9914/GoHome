#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Pause/PauseUIStyle.h"
#include "PauseConfirmDialog.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UWidgetAnimation;

USTRUCT(BlueprintType)
struct FPauseConfirmContent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Pause")
	FText Eyebrow;

	UPROPERTY(BlueprintReadWrite, Category = "Pause")
	FText Heading;

	UPROPERTY(BlueprintReadWrite, Category = "Pause")
	FText Body;

	UPROPERTY(BlueprintReadWrite, Category = "Pause")
	FText ConfirmLabel;

	// 게임 종료처럼 되돌릴 수 없는 확정 — 확정 버튼/머리글을 호박색 경고 스타일로
	UPROPERTY(BlueprintReadWrite, Category = "Pause")
	bool bWarning = false;
};

/**
 * 메뉴 위에 뜨는 확인 팝업(scrim + 팝업). 취소/Escape는 팝업만 닫고, 확정은 소유자에게 알릴 뿐 직접 세션을 건드리지 않는다.
 * 세션 정리 중(Busy)에는 두 버튼을 잠그고, 실패하면 팝업 안에 오류를 띄운 채 확정 버튼을 "다시 시도"로 바꾼다.
 */
UCLASS(Abstract)
class GOHOME_API UPauseConfirmDialog : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE(FOnPauseConfirmAction);
	FOnPauseConfirmAction OnConfirmed;
	FOnPauseConfirmAction OnCancelled;

	void Open(const FPauseConfirmContent& Content);
	void Close();
	bool IsOpen() const { return bOpen; }

	void SetBusy(const FText& StatusMessage);
	void ShowError(const FText& Message);
	bool IsBusy() const { return bBusy; }

	// Escape / 게임패드 B — 정리 중이 아니면 취소와 같은 경로
	void HandleBack();

	void FocusDefault();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EyebrowText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HeadingText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ConfirmLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CancelLabel;

	// 진행 중 / 오류 안내 한 줄
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	// 팝업 상단 2px 호박색 선
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AccentLine;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> OpenAnim;

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText RetryLabel = NSLOCTEXT("PauseMenu", "Retry", "다시 시도");

	// .confirm-button / .warning-button 배경
	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor ConfirmColor = PauseUIStyle::SRGBA(0xA3, 0xD3, 0xC5);

	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor WarningColor = PauseUIStyle::SRGBA(0xD6, 0xAB, 0x76);

	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor EyebrowColor = PauseUIStyle::SRGBA(0xA9, 0xBC, 0xB2);

	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor WarningEyebrowColor = PauseUIStyle::SRGBA(0xD6, 0xA8, 0x75);

	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor StatusColor = PauseUIStyle::SRGBA(0xB2, 0xC0, 0xB9);

	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor ErrorColor = PauseUIStyle::SRGBA(0xE8, 0x9A, 0x82);

	// .quiet-button / :hover·:focus-visible
	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor CancelColor = PauseUIStyle::SRGBA(0xB2, 0xC1, 0xB9);

	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	FLinearColor CancelFocusColor = FLinearColor::White;

	// 확정 버튼 포커스 시 배경을 이만큼 밝힌다(:focus-visible 대체 — Slate 버튼은 키보드 포커스를 그리지 않음)
	UPROPERTY(EditAnywhere, Category = "Pause|Style")
	float ConfirmFocusBrighten = 1.18f;

private:
	// 키보드/게임패드 포커스가 어느 버튼에 있는지 보이게 한다 — hover는 버튼 스타일이 처리.
	void RefreshFocusVisuals();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleConfirmClicked();

	void SetButtonsEnabled(bool bEnabled);

	FPauseConfirmContent CurrentContent;
	bool bOpen = false;
	bool bBusy = false;
};
