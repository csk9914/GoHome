#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/WeakInterfacePtr.h"
#include "UI/Title/TitleUITypes.h"
#include "TitleScreenWidget.generated.h"

class ITitleBackend;
class UButton;
class UTitleMenuItemWidget;
class UTitleMenuWidget;
class UTitlePanelBase;
class UTitleToastWidget;
class UWidgetAnimation;

/**
 * 타이틀 화면 루트(Fullscreen). 오너 PlayerController를 ITitleBackend로 받아 패널에 넘기고,
 * 메뉴 → 패널 라우팅, 패널 1단 스택과 포커스 복귀, Escape/게임패드 B를 소유한다. 연출 타이밍은 BP 애니메이션.
 */
UCLASS(Abstract)
class GOHOME_API UTitleScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	ITitleBackend* GetBackend() const { return Backend.Get(); }

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTitleMenuWidget> MainMenu;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTitlePanelBase> HostPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTitlePanelBase> SessionBrowserPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTitlePanelBase> SettingsPanel;

	// 패널이 열린 동안 뒤 화면을 어둡게 하고 입력을 막는다. 클릭하면 패널을 닫는다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Shade;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTitleToastWidget> Toast;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> IntroAnim;

	UPROPERTY(EditAnywhere, Category = "Title|Text")
	FText HostSuccessMessage = NSLOCTEXT("Title", "ToastHostSuccess", "세션이 준비되었습니다. 로비로 이동합니다.");

	UPROPERTY(EditAnywhere, Category = "Title|Text")
	FText HostFailureMessage = NSLOCTEXT("Title", "ToastHostFailure", "세션을 만들지 못했습니다. 다시 시도하세요.");

	UPROPERTY(EditAnywhere, Category = "Title|Text")
	FText JoinSuccessMessage = NSLOCTEXT("Title", "ToastJoinSuccess", "세션에 참가합니다.");

private:
	void HandleMenuItemActivated(UTitleMenuItemWidget* Item);
	void HandlePanelClosed(UTitlePanelBase* Panel);

	UFUNCTION()
	void HandleShadeClicked();

	void HandleHostComplete(bool bWasSuccessful);
	void HandleJoinComplete(bool bWasSuccessful);
	void ShowToast(const FText& Message);

	UTitlePanelBase* FindPanel(ETitleMenuAction Action) const;
	void OpenPanel(UTitlePanelBase* Panel, UTitleMenuItemWidget* Source);
	void SetShadeVisible(bool bVisible);

	TWeakInterfacePtr<ITitleBackend> Backend;
	TWeakObjectPtr<UTitlePanelBase> ActivePanel;
	TWeakObjectPtr<UTitleMenuItemWidget> ReturnFocusItem;
	FDelegateHandle HostCompleteHandle;
	FDelegateHandle JoinCompleteHandle;
};
