#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/WeakInterfacePtr.h"
#include "TitlePanelBase.generated.h"

class ITitleBackend;
class UButton;
class UWidgetAnimation;

/**
 * 타이틀 우측 패널(HOST / FIND / 설정)의 공통 수명주기 — 템플릿 메서드.
 * 열기/닫기 애니메이션은 BP가 소유(없으면 즉시 완료 = reduced-motion 경로), 닫는 순간 입력을 잠그고,
 * 닫힘이 끝나면 OnPanelClosed로 알려 화면이 포커스를 트리거 항목에 되돌리게 한다.
 */
UCLASS(Abstract)
class GOHOME_API UTitlePanelBase : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPanelClosed, UTitlePanelBase* /*Panel*/);
	FOnPanelClosed OnPanelClosed;

	void BindBackend(ITitleBackend* InBackend);

	void OpenPanel();
	void RequestClose();

	bool IsOpenOrOpening() const { return PanelState == EPanelState::Opening || PanelState == EPanelState::Open; }

	// Escape / 게임패드 B. 기본은 닫기 — 패널 내부에 한 단계 뒤로가 있으면(예: 초기화 확인 → 선택) 하위 클래스가 가로챈다.
	virtual void HandleBack();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	ITitleBackend* GetBackend() const { return Backend.Get(); }

	// 열림이 끝난 뒤 키보드 포커스를 받을 위젯. nullptr이면 패널 자체.
	virtual UWidget* GetInitialFocus() const { return nullptr; }

	virtual void OnBackendBound() {}
	virtual void OnPanelOpening() {}
	virtual void OnPanelClosing() {}

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> OpenAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> CloseAnim;

private:
	enum class EPanelState : uint8
	{
		Closed,
		Opening,
		Open,
		Closing,
	};

	UFUNCTION()
	void HandleCloseClicked();

	void FinishOpen();
	void FinishClose();

	EPanelState PanelState = EPanelState::Closed;
	TWeakInterfacePtr<ITitleBackend> Backend;
};
