#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/WeakInterfacePtr.h"
#include "Core/PauseMenuBackend.h"
#include "PauseMenuWidget.generated.h"

class IPauseMenuBackend;
class UButton;
class UPanelWidget;
class UPauseConfirmDialog;
class UPauseMenuItemWidget;
class UTitlePanelBase;
class UTitleToastWidget;
class UWidgetAnimation;

/**
 * 로비·탐사 공용 인게임 시스템 메뉴(Modal) 루트. 오너 PlayerController를 IPauseMenuBackend로 받는다.
 * 메뉴 → 설정(같은 창 안에서 교체) / 확인 팝업(메뉴 위) 라우팅, Escape·B 한 단계 뒤로, 포커스 복귀를 소유한다.
 * 위젯 생성·입력 모드·세션 정리는 백엔드 몫 — 이 위젯은 요청만 하고 결과(실패)만 받는다. 연출 타이밍은 BP 애니메이션.
 */
UCLASS(Abstract)
class GOHOME_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	// 메뉴 창(헤더·목록/설정·푸터). 확인 팝업이 떠 있는 동안 입력·포커스 이동 대상에서 빠진다(inert).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> MenuWindow;

	// 메뉴 목록 영역 — 설정이 열리면 같은 창 안에서 접힌다
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> MenuView;

	// UPauseMenuItemWidget 행들의 부모. 항목 순서는 BP 자식 순서가 출처.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ItemContainer;

	// 타이틀과 같은 설정 패널 클래스(UTitleSettingsPanel) — 백엔드 없이 동작하고 열기/닫기/뒤로 수명주기가 같다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTitlePanelBase> SettingsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPauseConfirmDialog> ConfirmDialog;

	// 헤더 "ESC ×" — 게임으로 복귀
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTitleToastWidget> Toast;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> OpenAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> CloseAnim;

	// --- 확인 팝업 문구(역할별) ---
	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText TitleEyebrow = NSLOCTEXT("PauseMenu", "TitleEyebrow", "SESSION / LEAVE");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText TitleHeading = NSLOCTEXT("PauseMenu", "TitleHeading", "타이틀 화면으로 이동할까요?");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText TitleBodyHost = NSLOCTEXT("PauseMenu", "TitleBodyHost", "세션이 종료되어 함께 플레이 중인 인원의 연결도 끊깁니다. 타이틀 화면으로 이동합니다.");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText TitleBodyParticipant = NSLOCTEXT("PauseMenu", "TitleBodyParticipant", "현재 세션에서 나가고 타이틀 화면으로 이동합니다.");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText TitleConfirmLabel = NSLOCTEXT("PauseMenu", "TitleConfirm", "타이틀로 나가기");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText QuitEyebrow = NSLOCTEXT("PauseMenu", "QuitEyebrow", "SYSTEM / EXIT");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText QuitHeading = NSLOCTEXT("PauseMenu", "QuitHeading", "게임을 종료할까요?");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText QuitBodyHost = NSLOCTEXT("PauseMenu", "QuitBodyHost", "세션을 종료하고 게임을 닫습니다. 함께 플레이 중인 인원의 연결도 끊깁니다.");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText QuitBodyParticipant = NSLOCTEXT("PauseMenu", "QuitBodyParticipant", "현재 세션에서 나간 뒤 게임을 종료합니다.");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText QuitConfirmLabel = NSLOCTEXT("PauseMenu", "QuitConfirm", "게임 종료");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText BusyHost = NSLOCTEXT("PauseMenu", "BusyHost", "세션을 종료하는 중…");

	UPROPERTY(EditAnywhere, Category = "Pause|Text")
	FText BusyParticipant = NSLOCTEXT("PauseMenu", "BusyParticipant", "세션에서 나가는 중…");

private:
	void HandleItemActivated(UPauseMenuItemWidget* Item);
	void HandleSettingsClosed(UTitlePanelBase* Panel);
	void HandleToastRequested(const FText& Message);
	void HandleConfirmed();
	void HandleConfirmCancelled();
	void HandleLeaveFailed(EPauseLeaveTarget Target, const FText& Reason);

	UFUNCTION()
	void HandleCloseClicked();

	void OpenSettings(UPauseMenuItemWidget* Source);
	void OpenConfirm(EPauseLeaveTarget Target, UPauseMenuItemWidget* Source);
	void CloseConfirm();

	void BeginClose();
	void FinishClose();

	void FocusItem(UPauseMenuItemWidget* Item);
	UWidget* GetCurrentFocusTarget() const;
	bool IsHost() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPauseMenuItemWidget>> Items;

	TWeakInterfacePtr<IPauseMenuBackend> Backend;
	TWeakObjectPtr<UPauseMenuItemWidget> ReturnFocusItem;
	TOptional<EPauseLeaveTarget> ConfirmTarget;
	FDelegateHandle LeaveFailedHandle;
	bool bClosing = false;
};
