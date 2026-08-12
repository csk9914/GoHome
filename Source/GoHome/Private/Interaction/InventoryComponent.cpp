//THE

#include "Interaction/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemActorBase.h"
#include "Player/DeathNotifier.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IDeathNotifier* DeathNotifier = GetOwner()->FindComponentByInterface<IDeathNotifier>())
	{
		DeathNotifier->GetOnDeathDelegate().AddUObject(this, &UInventoryComponent::ServerDropAllItems);
	}
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

void UInventoryComponent::ServerDropAllItems()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (FInventorySlot& Slot : Slots)
	{
		if (Slot.Item)
		{
			Slot.Item->ServerDrop();
		}
	}
}

void UInventoryComponent::TryDropItem(AItemActorBase* ItemToDrop)
{
	if (!ItemToDrop) return;
	Server_RequestDrop(ItemToDrop);
}

void UInventoryComponent::Server_RequestDrop_Implementation(AItemActorBase* ItemToDrop)
{
	if (!ItemToDrop) return;

	// 클라이언트 요청을 그대로 신뢰하지 않음 -> 실제 내 인벤토리에 있는 아이템인지 검증함.
	bool bOwnsItem = false;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Item == ItemToDrop)
		{
			bOwnsItem = true;
			break;
		}
	}

	if (!bOwnsItem) return;

	ItemToDrop->ServerDrop();
}

AItemActorBase* UInventoryComponent::GetItemInSlot(int32 SlotIndex) const
{
	// Slots는 고정 C 배열이라 수동 범위 체크.
	if (SlotIndex < 0 || SlotIndex >= GoHomeInventorySlotCount) return nullptr;
	return Slots[SlotIndex].Item;
}
