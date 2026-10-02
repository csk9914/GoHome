#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class AItemActorBase;
class UBorder;
class UImage;
class UTextBlock;
class UWidget;

/**
 * 핫바 슬롯 한 칸(Reference Pack: Docs/Dev/UI/ingame-hud B안).
 * 좌상단 키 번호, 가운데 아이콘, 우하단 무게. 활성 칸은 청록 테두리 + 위로 4px(120ms ease-out).
 * 빈 칸은 점선 자리(EmptyMark)만. 레이아웃·스타일은 BP(WBP_InventorySlot).
 */
UCLASS(Abstract)
class GOHOME_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSlot(int32 InSlotIndex, const AItemActorBase* Item, bool bInActive);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> SlotBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EmptyMark;

	// 아이콘이 없는 아이템(ItemData.Icon 미지정)용 — 이름 앞 2글자
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FallbackText;

	UPROPERTY(EditAnywhere, Category = "Slot|Style")
	FLinearColor IdleBorderColor = FLinearColor(0.85f, 0.92f, 0.92f, 0.14f);

	UPROPERTY(EditAnywhere, Category = "Slot|Style")
	FLinearColor ActiveBorderColor = FLinearColor::FromSRGBColor(FColor(0x19, 0xE3, 0xEE));

	UPROPERTY(EditAnywhere, Category = "Slot|Style")
	FLinearColor IdleKeyColor = FLinearColor::FromSRGBColor(FColor(0x7E, 0x9A, 0x9A));

	UPROPERTY(EditAnywhere, Category = "Slot|Style")
	float IdleBorderWidth = 1.f;

	UPROPERTY(EditAnywhere, Category = "Slot|Style")
	float ActiveBorderWidth = 2.f;

	UPROPERTY(EditAnywhere, Category = "Slot|Motion")
	float ActiveLift = 4.f;

	UPROPERTY(EditAnywhere, Category = "Slot|Motion", meta = (ClampMin = "0"))
	float ActiveSeconds = 0.12f;

private:
	void ApplyBorder(float ActiveAlpha);

	bool bActive = false;
	// 0 = 비활성 모습, 1 = 활성 모습. ActiveSeconds 동안 목표로 이동
	float ActiveAlpha = 0.f;
	bool bHasAppliedState = false;
};
