#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Title/TitleUITypes.h"
#include "TitleMenuWidget.generated.h"

class UPanelWidget;
class UTitleMenuItemWidget;

/**
 * 세로 메뉴. 항목 목록은 BP의 ItemContainer 자식 순서가 출처(항목 추가는 레이아웃 작업).
 * 진입 시 아무 항목도 선택하지 않는다 — 첫 방향키 입력에서 처음/마지막 항목으로 들어온다.
 */
UCLASS(Abstract)
class GOHOME_API UTitleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMenuItemActivated, UTitleMenuItemWidget* /*Item*/);
	FOnMenuItemActivated OnItemActivated;

	bool HasHighlightedItem() const;
	void FocusFirstItem();
	void FocusLastItem();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ItemContainer;

private:
	void HandleItemActivated(UTitleMenuItemWidget* Item);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTitleMenuItemWidget>> Items;
};
