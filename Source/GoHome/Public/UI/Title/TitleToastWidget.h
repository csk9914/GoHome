#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleToastWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/** 작업 결과 안내. 입력을 막지 않고, 새 메시지는 기존 메시지를 교체한다(큐잉하지 않음 — 결과 안내는 최신 것만 의미 있음). */
UCLASS(Abstract)
class GOHOME_API UTitleToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Title|Toast")
	void ShowMessage(const FText& Message);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	UPROPERTY(EditAnywhere, Category = "Title|Toast")
	float HoldSeconds = 2.7f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ShowAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> HideAnim;

private:
	void Hide();

	FTimerHandle HoldTimer;
};
