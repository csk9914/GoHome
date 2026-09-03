//THE

#include "Interaction/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemActorBase.h"
#include "Item/FlashlightActor.h"
#include "Item/UsableItemBase.h"
#include "Player/DeathNotifier.h"
#include "Player/GoHomeCharacter.h"
#include "Interaction/CoopCarryObjectBase.h"
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
	OnInventoryChanged.Broadcast();
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
	OnInventoryChanged.Broadcast(); // 추가: 서버 자신의 UI도 즉시 갱신함.
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
			OnInventoryChanged.Broadcast();
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
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	AGoHomeGameState* GameState = GetWorld()->GetGameState<AGoHomeGameState>();
	if (!GameState) return;

	// 정산으로 파괴되는 아이템은 ServerDrop()을 안거치기 때문에, 그게 활성(손에 든) 아이템이었다면
	// DetachItemFromRightHand()가 안 불려서 캐릭터의 bIsHoldingItem이 안 풀리는 버그가 있었음.
	// -> 오브젝트는 사라졌는데 들고 있는 모션만 그대로 남음. 활성 아이템이 실제로 정산 대상이었을 때만 리셋.
	AItemActorBase* PreviousActiveItem = GetActiveItem();
	bool bActiveItemDelivered = false;

	for (FInventorySlot& Slot : Slots)
	{
		AItemActorBase* Item = Slot.Item;
		if (!Item || !Item->IsDeliverable()) continue;

		if (Item == PreviousActiveItem)
		{
			bActiveItemDelivered = true;
		}

		GameState->AddDeliveredValue(FMath::RoundToInt(Item->GetCurrentValue()));
		// 슬롯 비우기 + NotifyDropped() (소음 타이머 정지).
		RemoveItem(Item);
		Item->Destroy();
	}

	if (bActiveItemDelivered)
	{
		if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(GetOwner()))
		{
			Character->DetachItemFromRightHand();
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

void UInventoryComponent::TryDropOrReleaseCarry()
{
	if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(GetOwner()))
	{
		if (Character->IsCoopCarrying())
		{
			Server_RequestReleaseCarry();
			return;
		}
	}

	TryDropItem(GetActiveItem());
}

void UInventoryComponent::Server_RequestReleaseCarry_Implementation()
{
	if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(GetOwner()))
	{
		if (ACoopCarryObjectBase* CarryObject = Character->GetCurrentCarryObject())
		{
			CarryObject->ReleaseCarriers();
		}
	}
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
	OnInventoryChanged.Broadcast();
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
	OnInventoryChanged.Broadcast();
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

void UInventoryComponent::TryToggleFlashlight()
{
	Server_RequestToggleFlashlight();
}

void UInventoryComponent::Server_RequestToggleFlashlight_Implementation()
{
	// Spot은 최대 1개만 소지 가능하니, 찾으면 그걸로 끝.
	for (const FInventorySlot& Slot : Slots)
	{
		if (AFlashlightActor* Flashlight = Cast<AFlashlightActor>(Slot.Item))
		{
			Flashlight->ServerUseSpecialAction();
			break;
		}
	}
}

void UInventoryComponent::TryUseActiveItem()
{
	Server_RequestUseActiveItem();
}

void UInventoryComponent::Server_RequestUseActiveItem_Implementation()
{
	if (AUsableItemBase* UsableItem = Cast<AUsableItemBase>(GetActiveItem()))
	{
		if (UsableItem->CanUse())
		{
			UsableItem->ServerUseSpecialAction();
		}
	}
}