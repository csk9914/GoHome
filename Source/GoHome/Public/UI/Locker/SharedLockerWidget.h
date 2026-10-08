#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SharedLockerWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class USharedLockerRowWidget;
class AGoHomeGameState;
struct FSharedLockerResult;

/**
 * 잠수정 공유 보관함 Modal 루트. 생성·입력 모드는 오너 PlayerController(ISharedLockerBackend)가 소유한다.
 * 수량은 AGoHomeGameState 복제 미러(OnSharedLockerChanged)만 구독해 그리고, 요청 결과 문구는 백엔드 이벤트로 받는다.
 * 이 위젯은 게임 상태를 직접 바꾸지 않는다.
 */
UCLASS(Abstract)
class GOHOME_API USharedLockerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 행이 채워질 컨테이너(VerticalBox/ScrollBox 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> EntryList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(EditDefaultsOnly, Category = "Shared Locker")
	TSubclassOf<USharedLockerRowWidget> RowClass;

private:
	UFUNCTION()
	void HandleLockerChanged();

	UFUNCTION()
	void HandleCloseClicked();

	void HandleLockerResult(const FSharedLockerResult& Result);

	void RequestClose();

	TWeakObjectPtr<AGoHomeGameState> BoundGameState;
	FDelegateHandle ResultHandle;
};
