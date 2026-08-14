//THE

#include "Interaction/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemActorBase.h"
#include "Player/DeathNotifier.h"
#include "Core/GoHomeGameState.h"

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
	DOREPLIFETIME(UInventoryComponent, ActiveSlotIndex);
}

void UInventoryComponent::OnRep_Slots()
{
}


bool UInventoryComponent::TryAddItem(AItemActorBase* Item)
{
	if (!Item) return false;

	// 이미 인벤토리 어딘가에 있는 아이템이면 중복 등록 방지.

	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Item == Item) return false;
	}

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

void UInventoryComponent::ServerDeliverAllItems()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("ServerDeliverAllItems 진입"));
	}

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	AGoHomeGameState* GameState = GetWorld()->GetGameState<AGoHomeGameState>();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
			FString::Printf(TEXT("GameState = %s"), GameState ? *GameState->GetName() : TEXT("null")));
	}

	if (!GameState) return;

	for (FInventorySlot& Slot : Slots)
	{
		AItemActorBase* Item = Slot.Item;
		if (!Item) continue;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
				FString::Printf(TEXT("정산 처리 중: %s"), *Item->GetName()));
		}

		GameState->AddDeliveredValue(FMath::RoundToInt(Item->GetCurrentValue()));
		// 슬롯 비우기 + NotifyDropped() (소음 타이머 정지).
		RemoveItem(Item);
		Item->Destroy();
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


void UInventoryComponent::SetActiveSlot(int32 NewIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (NewIndex < 0 || NewIndex >= GoHomeInventorySlotCount) return;

	AItemActorBase* OldActive = GetItemInSlot(ActiveSlotIndex);
	AItemActorBase* NewActive = GetItemInSlot(NewIndex);

	if (OldActive && OldActive != NewActive)
	{
		OldActive->SetActiveHeld(false);
	}

	ActiveSlotIndex = NewIndex;

	if (NewActive)
	{
		NewActive->SetActiveHeld(true);
	}
}

void UInventoryComponent::TrySetActiveSlot(int32 NewIndex)
{
	Server_RequestSetActiveSlot(NewIndex);
}

void UInventoryComponent::Server_RequestSetActiveSlot_Implementation(int32 NewIndex)
{
	SetActiveSlot(NewIndex);
}

void UInventoryComponent::OnRep_ActiveSlotIndex()
{
}

AItemActorBase* UInventoryComponent::GetActiveItem() const
{
	return GetItemInSlot(ActiveSlotIndex);
}

int32 UInventoryComponent::FindSlotIndexOf(AItemActorBase* Item) const
{
	for (int32 Index = 0; Index < GoHomeInventorySlotCount; ++Index)
	{
		if (Slots[Index].Item == Item) return Index;
	}
	return INDEX_NONE;
}