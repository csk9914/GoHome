#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/SharedLockerTypes.h"
#include "SharedLockerRowWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * 보관함 상품 한 줄: 이름·종류(장비/소모품)·보관/보유 수량 + 꺼내기/넣기 버튼.
 * 버튼은 오너 PlayerController의 ISharedLockerBackend 요청만 보낸다 — 수량은 서버 복제가 다시 Setup으로 밀어 준다.
 */
UCLASS(Abstract)
class GOHOME_API USharedLockerRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FSharedLockerViewEntry& InEntry, const FText& InDisplayName, UTexture2D* InIcon);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> WithdrawButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DepositButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LifetimeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

private:
	UFUNCTION()
	void HandleWithdrawClicked();

	UFUNCTION()
	void HandleDepositClicked();

	FName ProductId = NAME_None;
};
