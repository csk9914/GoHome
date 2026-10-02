#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryHotbarWidget.generated.h"

class UInventoryComponent;
class UInventorySlotWidget;
class UPanelWidget;
class UTextBlock;

/**
 * 하단 중앙 인벤토리 핫바(Reference Pack: Docs/Dev/UI/ingame-hud B안).
 * 4칸 + 활성 아이템 이름·가치 라벨. UInventoryComponent::OnInventoryChanged 하나로 갱신(활성 슬롯 변경도 같은 이벤트).
 * 소유·생성은 기존대로 캐릭터 BP가 InitInventory 호출(이주는 범위 밖). 레이아웃은 BP(WBP_InventorySlots).
 */
UCLASS(Abstract)
class GOHOME_API UInventoryHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitInventory(UInventoryComponent* InInventory);

protected:
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActiveNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActiveValueText;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInventorySlotWidget> SlotClass;

	// 슬롯 한 칸 크기 — 런타임 SizeBox로 감싸 강제(슬롯 위젯 자체 desired에 의존하지 않음)
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	float SlotPixelSize = 112.f;

	// 슬롯 사이 간격(Pack 8px)
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "0"))
	float SlotGap = 8.f;

	UPROPERTY(EditAnywhere, Category = "Inventory|Text")
	FText EmptySlotText = NSLOCTEXT("Hotbar", "EmptySlot", "빈 슬롯");

	UPROPERTY(EditAnywhere, Category = "Inventory|Text")
	FText ValueFormat = NSLOCTEXT("Hotbar", "Value", "가치 {0}");

	// motion.md: 이름 교체 opacity 0→1
	UPROPERTY(EditAnywhere, Category = "Inventory|Motion", meta = (ClampMin = "0"))
	float NameFadeSeconds = 0.15f;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void BuildSlots();

	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgetList;

	int32 LastActiveIndex = INDEX_NONE;
	float NameFadeElapsed = 0.f;
};
