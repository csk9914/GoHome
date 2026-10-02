#include "UI/HUD/InventoryHotbarWidget.h"

#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Interaction/InventoryComponent.h"
#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"
#include "UI/HUD/InventorySlotWidget.h"

void UInventoryHotbarWidget::InitInventory(UInventoryComponent* InInventory)
{
	if (UInventoryComponent* Old = BoundInventory.Get())
	{
		Old->OnInventoryChanged.RemoveDynamic(this, &UInventoryHotbarWidget::HandleInventoryChanged);
	}

	BoundInventory = InInventory;
	if (InInventory)
	{
		InInventory->OnInventoryChanged.AddUniqueDynamic(this, &UInventoryHotbarWidget::HandleInventoryChanged);
	}

	BuildSlots();
	HandleInventoryChanged();
}

void UInventoryHotbarWidget::NativeDestruct()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryHotbarWidget::HandleInventoryChanged);
	}
	Super::NativeDestruct();
}

void UInventoryHotbarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (NameFadeElapsed < NameFadeSeconds)
	{
		NameFadeElapsed += InDeltaTime;
		const float Opacity = NameFadeSeconds > 0.f ? FMath::Clamp(NameFadeElapsed / NameFadeSeconds, 0.f, 1.f) : 1.f;
		ActiveNameText->SetRenderOpacity(Opacity);
		if (ActiveValueText)
		{
			ActiveValueText->SetRenderOpacity(Opacity);
		}
	}
}

void UInventoryHotbarWidget::BuildSlots()
{
	SlotContainer->ClearChildren();
	SlotWidgetList.Reset();

	const UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !SlotClass)
	{
		return;
	}

	for (int32 Index = 0; Index < Inventory->GetInventorySlotCount(); ++Index)
	{
		if (UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, SlotClass))
		{
			USizeBox* SlotBox = NewObject<USizeBox>(this);
			SlotBox->SetWidthOverride(SlotPixelSize);
			SlotBox->SetHeightOverride(SlotPixelSize);
			SlotBox->AddChild(SlotWidget);
			UPanelSlot* PanelSlot = SlotContainer->AddChild(SlotBox);
			if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
			{
				BoxSlot->SetPadding(FMargin(Index > 0 ? SlotGap : 0.f, 0.f, 0.f, 0.f));
				// 활성 칸이 위로 뜨는 만큼 아래 기준 정렬
				BoxSlot->SetVerticalAlignment(VAlign_Bottom);
			}
			SlotWidgetList.Add(SlotWidget);
		}
	}
}

void UInventoryHotbarWidget::HandleInventoryChanged()
{
	const UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	const int32 ActiveIndex = Inventory->ActiveSlotIndex;
	for (int32 Index = 0; Index < SlotWidgetList.Num(); ++Index)
	{
		SlotWidgetList[Index]->SetSlot(Index, Inventory->GetItemInSlot(Index), Index == ActiveIndex);
	}

	const AItemActorBase* ActiveItem = Inventory->GetItemInSlot(ActiveIndex);
	const UItemDataAsset* Data = ActiveItem ? ActiveItem->ItemData.Get() : nullptr;
	ActiveNameText->SetText(Data ? Data->DisplayName : EmptySlotText);
	if (ActiveValueText)
	{
		ActiveValueText->SetText(ActiveItem ? FText::Format(ValueFormat, FText::AsNumber(FMath::RoundToInt(ActiveItem->GetCurrentValue()))) : FText::GetEmpty());
		ActiveValueText->SetVisibility(ActiveItem ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ActiveIndex != LastActiveIndex)
	{
		LastActiveIndex = ActiveIndex;
		NameFadeElapsed = 0.f;
	}
}
