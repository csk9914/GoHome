//THE

#include "Interaction/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemActorBase.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

float UInventoryComponent::GetTotalWeight() const
{
	float TotalWeight = 0.f;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Item)
		{
			TotalWeight += Slot.Item->GetTotalWeight() * Slot.Quantity;

		}
	}
	return TotalWeight;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Slots);
}

void UInventoryComponent::OnRep_Slots()
{
}


bool UInventoryComponent::TryAddItem(AItemActorBase* Item)
{
	if (!Item) return false;

	const int32 EmptyIndex = FindEmptySlotIndex();
	if (EmptyIndex == INDEX_NONE)
	{
		return false;
	}

	Slots[EmptyIndex].Item = Item;
	Slots[EmptyIndex].Quantity = 1;
	Item->NotifyPickedUp();
	return true;
}

bool UInventoryComponent::RemoveItem(AItemActorBase* Item)
{
	if (!Item) return false;

	for (FInventorySlot& Slot : Slots)
	{
		if (Slot.Item == Item)
		{
			Slot.Item = nullptr;
			Slot.Quantity = 0;
			Item->NotifyDropped();
			return true;
		}
	}
	return false;
}


int32 UInventoryComponent::FindEmptySlotIndex() const
{
	for (int32 Index = 0; Index < GoHomeInventorySlotCount; ++Index)
	{
		if (Slots[Index].Item == nullptr)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}
