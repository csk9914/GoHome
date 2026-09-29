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

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> IntroAnim;

private:
	void HandleMenuItemActivated(UTitleMenuItemWidget* Item);
	void HandlePanelClosed(UTitlePanelBase* Panel);

	UFUNCTION()
	void HandleShadeClicked();

	UTitlePanelBase* FindPanel(ETitleMenuAction Action) const;
	void OpenPanel(UTitlePanelBase* Panel, UTitleMenuItemWidget* Source);
	void SetShadeVisible(bool bVisible);

	TWeakInterfacePtr<ITitleBackend> Backend;
	TWeakObjectPtr<UTitlePanelBase> ActivePanel;
	TWeakObjectPtr<UTitleMenuItemWidget> ReturnFocusItem;
};
