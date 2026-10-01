#include "Shop/ItemShopSubsystem.h"
#include "Shop/ItemShopCatalogDataAsset.h"
#include "Shop/ItemShopTypes.h"
#include "Core/GoHomeGameState.h"
#include "Core/ExpeditionState.h"
#include "Core/ExplorationGameState.h"
#include "Save/GoHomeSaveSubsystem.h"
#include "Interaction/InventoryComponent.h"
#include "UObject/UObjectGlobals.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Item/ItemActorBase.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
	const TCHAR* ItemShopCatalogPath =
		TEXT("/Game/GoHome/Developers/LSA/Data/DA_ItemShopCatalog.DA_ItemShopCatalog");

	FString MakeShopOwnerPlayerKey(
		const APlayerController* Requester)
	{
		if (!Requester)
		{
			return FString();
		}

		const APlayerState* PlayerState =
			Requester->GetPlayerState<APlayerState>();

		if (!PlayerState)
		{
			return FString();
		}

		// PIE에서 온라인 ID가 없을 때 사용할 임시 키
		return FString::Printf(
			TEXT("Player_%d"),
			PlayerState->GetPlayerId());
	}
}

void UItemShopSubsystem::Initialize(
	FSubsystemCollectionBase& CollectionBase
)
{
	Super::Initialize(CollectionBase);

	Catalog = LoadObject<UItemShopCatalogDataAsset>(
		nullptr,
		ItemShopCatalogPath
	);

	if (!Catalog)
	{
		// 상점 데이터가 없어도 기존 게임은 계속 실행되어야 함.
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ItemShop] Catalog faild. shop is envailable.")
		);

		bShopAvailable = false;
		return;
	}

	bShopAvailable = true;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[ItemShop] Catalog Loard Success. product count: %d"),
		Catalog->Products.Num()
	);
}

void UItemShopSubsystem::Deinitialize()
{
	Catalog = nullptr;
	bShopAvailable = false;

	Super::Deinitialize();
}

const FItemShopProduct* UItemShopSubsystem::FindProduct(
	FName ProductId
) const
{
	if (!bShopAvailable || !Catalog)
	{
		return nullptr;
	}

	return Catalog->FindProduct(ProductId);
}

bool UItemShopSubsystem::TryCalculateOrderTotal(
	const FItemShopPurchaseRequest& Request,
	int32& OutTotalPrice,
	EItemShopResult& OutResult
) const
{
	OutTotalPrice = 0;
	OutResult = EItemShopResult::ShopUnavailable;

	// 카탈로그가 없으면 상점만 사용할 수 없게 한다.
	if (!bShopAvailable || !Catalog)
	{
		return false;
	}

	// 현재는 탐사 중에만 상점 구매를 허용한다.
	UWorld* World = GetWorld();

	const AGoHomeGameState* GameState =
		World ? World->GetGameState<AGoHomeGameState>() : nullptr;

	const bool bIsExploration =
		GameState &&
		GameState->GetCurrentState() == EExpeditionState::Exploration;

	// 클라이언트가 보낸 Context만 믿지 않고,
	// 서버의 실제 GameState도 함께 확인한다.
	if (!bIsExploration ||
		Request.Context != EItemShopContext::InGameImmediate)
	{
		OutResult = EItemShopResult::NotAllowedInContext;
		return false;
	}

	// 장바구니가 비어 있으면 잘못된 주문이다.
	if (Request.Lines.Num() == 0)
	{
		OutResult = EItemShopResult::InvalidQuantity;
		return false;
	}

	for (const FItemShopCartLine& Line : Request.Lines)
	{
		if (Line.ProductId.IsNone())
		{
			OutResult = EItemShopResult::InvalidProduct;
			return false;
		}

		if (Line.Quantity <= 0)
		{
			OutResult = EItemShopResult::InvalidQuantity;
			return false;
		}

		const FItemShopProduct* Product =
			FindProduct(Line.ProductId);

		if (!Product)
		{
			OutResult = EItemShopResult::InvalidProduct;
			return false;
		}

		if (!Product->ItemData || !Product->ActorClass)
		{
			OutResult = EItemShopResult::InvalidProduct;
			return false;
		}

		if (Product->PurchasePrice < 0)
		{
			OutResult = EItemShopResult::InvalidProduct;
			return false;
		}

		if (Line.Quantity > Product->MaxQuantityPerOrder)
		{
			OutResult = EItemShopResult::InvalidQuantity;
			return false;
		}

		const bool bAllowedInContext =
			Request.Context == EItemShopContext::LobbyLoadout
			? Product->bCanBuyInLobby
			: Product->bCanBuyInGame;

		if (!bAllowedInContext)
		{
			OutResult = EItemShopResult::NotAllowedInContext;
			return false;
		}

		const int64 LinePrice =
			static_cast<int64>(Product->PurchasePrice)
			* static_cast<int64>(Line.Quantity);

		const int64 NewTotal =
			static_cast<int64>(OutTotalPrice)
			+ LinePrice;

		// 가격 계산이 int32 범위를 넘지 않는지 확인한다.
		if (NewTotal > MAX_int32)
		{
			OutResult = EItemShopResult::InvalidQuantity;
			return false;
		}

		OutTotalPrice = static_cast<int32>(NewTotal);
	}

	OutResult = EItemShopResult::Success;
	return true;
}

bool UItemShopSubsystem::TryValidatePurchase(
	const FItemShopPurchaseRequest& Request,
	int32& OutTotalPrice,
	int32& OutCurrentFunds,
	EItemShopResult& OutResult
) const
{
	OutTotalPrice = 0;
	OutCurrentFunds = 0;
	OutResult = EItemShopResult::ShopUnavailable;

	// 먼저 상품, 수량, 가격, 구매 장소를 확인한다.
	if (!TryCalculateOrderTotal(
		Request,
		OutTotalPrice,
		OutResult))
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		OutResult = EItemShopResult::InvalidRequester;
		return false;
	}

	UGoHomeSaveSubsystem* SaveSubsystem =
		GameInstance->GetSubsystem<UGoHomeSaveSubsystem>();

	if (!SaveSubsystem)
	{
		OutResult = EItemShopResult::ShopUnavailable;
		return false;
	}

	// 현재 보유 코인을 가져온다.
	OutCurrentFunds = SaveSubsystem->GetCurrentFunds();

	// 코인이 부족하면 구매 실패.
	if (OutCurrentFunds < OutTotalPrice)
	{
		OutResult = EItemShopResult::NotEnoughFunds;
		return false;
	}

	// 여기까지 통과하면 구매 조건은 만족한 상태.
	OutResult = EItemShopResult::Success;
	return true;
}

void UItemShopSubsystem::RegisterRuntimeShopItem(
	AItemActorBase* Item,
	UInventoryComponent* OwnerInventory,
	FName ProductId,
	const FString& OwnerPlayerKey)
{
	if (!Item || !OwnerInventory || ProductId.IsNone())
	{
		return;
	}

	FItemShopRuntimeItem RuntimeItem;

	RuntimeItem.Item = Item;
	RuntimeItem.OwnerInventory = OwnerInventory;
	RuntimeItem.ProductId = ProductId;
	RuntimeItem.OwnerPlayerKey = OwnerPlayerKey;

	RuntimeShopItems.Add(RuntimeItem);
}

void UItemShopSubsystem::UnregisterRuntimeShopItem(
	AItemActorBase* Item)
{
	if (!Item)
	{
		return;
	}

	RuntimeShopItems.RemoveAll(
		[Item](const FItemShopRuntimeItem& RuntimeItem)
		{
			return RuntimeItem.Item.Get() == Item;
		});
}

void UItemShopSubsystem::ClearRuntimeShopItems()
{
	RuntimeShopItems.Reset();
	LoadoutGrantedPlayers.Reset();
}

bool UItemShopSubsystem::TryGrantSavedLoadout(
	APlayerController* Requester)
{
	if (!Requester ||
		!Requester->HasAuthority() ||
		!Requester->GetPawn())
	{
		return false;
	}

	if (!bShopAvailable || !Catalog)
	{
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	// 탐사 맵에서만 지급한다.
	const AGoHomeGameState* GameState =
		World->GetGameState<AGoHomeGameState>();

	if (!GameState ||
		GameState->GetCurrentState() != EExpeditionState::Exploration)
	{
		return false;
	}

	const FString OwnerPlayerKey =
		MakeShopOwnerPlayerKey(Requester);

	if (OwnerPlayerKey.IsEmpty())
	{
		return false;
	}

	// SetPawn이 여러 번 호출되어도 중복 지급하지 않는다.
	if (LoadoutGrantedPlayers.Contains(OwnerPlayerKey))
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();

	if (!GameInstance)
	{
		return false;
	}

	UGoHomeSaveSubsystem* SaveSubsystem =
		GameInstance->GetSubsystem<UGoHomeSaveSubsystem>();

	if (!SaveSubsystem)
	{
		return false;
	}

	APawn* Pawn = Requester->GetPawn();

	UInventoryComponent* Inventory =
		Pawn->FindComponentByClass<UInventoryComponent>();

	if (!Inventory)
	{
		return false;
	}

	// 먼저 지급해야 할 전체 수량을 계산한다.
	int32 RequiredSlotCount = 0;

	for (const FItemShopProduct& Product : Catalog->Products)
	{
		if (Product.ProductId.IsNone())
		{
			continue;
		}

		const int32 OwnedQuantity =
			SaveSubsystem->GetShopOwnedQuantity(
				OwnerPlayerKey,
				Product.ProductId);

		if (OwnedQuantity <= 0)
		{
			continue;
		}

		if (!Product.ItemData ||
			!Product.ActorClass)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemShop] Loadout product data is invalid: %s"),
				*Product.ProductId.ToString());

			return false;
		}

		RequiredSlotCount += OwnedQuantity;
	}

	// 현재 인벤토리의 빈칸 수를 계산한다.
	int32 EmptySlotCount = 0;

	for (int32 SlotIndex = 0;
		SlotIndex < Inventory->GetInventorySlotCount();
		++SlotIndex)
	{
		if (!Inventory->GetItemInSlot(SlotIndex))
		{
			++EmptySlotCount;
		}
	}

	if (RequiredSlotCount > EmptySlotCount)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ItemShop] Loadout failed. Required: %d, Empty: %d"),
			RequiredSlotCount,
			EmptySlotCount);

		return false;
	}

	TArray<AItemActorBase*> GrantedItems;

	// 중간에 실패하면 지금 라운드에 만든 아이템을 전부 되돌린다.
	auto RollbackItems = [&]()
		{
			for (AItemActorBase* Item : GrantedItems)
			{
				if (!Item)
				{
					continue;
				}

				UnregisterRuntimeShopItem(Item);
				Item->ServerDrop();
				Item->Destroy();
			}

			GrantedItems.Reset();
		};

	// 상품별 저장 보유량만큼 아이템을 생성한다.
	for (const FItemShopProduct& Product : Catalog->Products)
	{
		const int32 OwnedQuantity =
			SaveSubsystem->GetShopOwnedQuantity(
				OwnerPlayerKey,
				Product.ProductId);

		for (int32 Index = 0;
			Index < OwnedQuantity;
			++Index)
		{
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(
				Pawn->GetActorLocation());
			SpawnTransform.SetRotation(
				Pawn->GetActorQuat());

			AItemActorBase* SpawnedItem =
				World->SpawnActorDeferred<AItemActorBase>(
					Product.ActorClass,
					SpawnTransform,
					Requester,
					Pawn,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (!SpawnedItem)
			{
				RollbackItems();
				return false;
			}

			// 아이템 데이터 설정
			SpawnedItem->ItemData =
				Product.ItemData;

			SpawnedItem->FinishSpawning(
				SpawnTransform);

			// 기존 픽업 로직을 이용해 인벤토리에 넣는다.
			SpawnedItem->OnInteract(Pawn);

			if (Inventory->FindSlotIndexOf(SpawnedItem) == INDEX_NONE)
			{
				SpawnedItem->Destroy();
				RollbackItems();
				return false;
			}

			// 다음 라운드 종료 때 보유 여부를 다시 확인할 수 있게 등록한다.
			RegisterRuntimeShopItem(
				SpawnedItem,
				Inventory,
				Product.ProductId,
				OwnerPlayerKey);

			GrantedItems.Add(SpawnedItem);
		}
	}

	// 이번 라운드에 이미 지급했다는 기록
	LoadoutGrantedPlayers.Add(OwnerPlayerKey);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[ItemShop] Loadout granted. Player: %s, Items: %d"),
		*OwnerPlayerKey,
		RequiredSlotCount);

	return true;
}

void UItemShopSubsystem::ReconcileRuntimeShopItems()
{
	UGameInstance* GameInstance = GetGameInstance();

	if (!GameInstance)
	{
		return;
	}

	UGoHomeSaveSubsystem* SaveSubsystem =
		GameInstance->GetSubsystem<UGoHomeSaveSubsystem>();

	if (!SaveSubsystem)
	{
		return;
	}

	// 플레이어별, 상품별로 현재 인벤토리에 남아 있는 수량을 센다.
	TMap<FString, TMap<FName, int32>> OwnedCounts;

	for (const FItemShopRuntimeItem& RuntimeItem : RuntimeShopItems)
	{
		if (RuntimeItem.OwnerPlayerKey.IsEmpty() ||
			RuntimeItem.ProductId.IsNone())
		{
			continue;
		}

		// 이 아이템이 장부에 있었다는 사실은 먼저 기록한다.
		// 나중에 아이템을 잃었으면 0으로 저장하기 위해서다.
		int32& OwnedCount =
			OwnedCounts
			.FindOrAdd(RuntimeItem.OwnerPlayerKey)
			.FindOrAdd(RuntimeItem.ProductId);

		UInventoryComponent* Inventory =
			RuntimeItem.OwnerInventory.Get();

		AItemActorBase* Item =
			RuntimeItem.Item.Get();

		// 실제로 현재 인벤토리에 있으면 보유 수량을 증가시킨다.
		if (Inventory &&
			Item &&
			Inventory->FindSlotIndexOf(Item) != INDEX_NONE)
		{
			++OwnedCount;
		}
	}

	// 계산한 현재 보유 수량을 세이브에 반영한다.
	for (const TPair<FString, TMap<FName, int32>>& OwnerPair : OwnedCounts)
	{
		for (const TPair<FName, int32>& ProductPair : OwnerPair.Value)
		{
			SaveSubsystem->SetShopOwnedQuantity(
				OwnerPair.Key,
				ProductPair.Key,
				ProductPair.Value);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[ItemShop] Runtime item ownership reconciled."));
}

bool UItemShopSubsystem::TryProcessPurchase(
	APlayerController* Requester,
	const FItemShopPurchaseRequest& Request,
	FItemShopPurchaseResult& OutResult)
{
	OutResult = FItemShopPurchaseResult();

	if (!Requester || !Requester->GetPawn())
	{
		OutResult.Result = EItemShopResult::InvalidRequester;
		OutResult.Message =
			FText::FromString(TEXT("구매자를 찾을 수 없습니다."));
		return false;
	}

	APawn* Pawn = Requester->GetPawn();

	UInventoryComponent* Inventory =
		Pawn->FindComponentByClass<UInventoryComponent>();

	if (!Inventory)
	{
		OutResult.Result = EItemShopResult::InvalidRequester;
		OutResult.Message =
			FText::FromString(TEXT("인벤토리를 찾을 수 없습니다."));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();

	if (!GameInstance)
	{
		OutResult.Result = EItemShopResult::ShopUnavailable;
		return false;
	}

	UGoHomeSaveSubsystem* SaveSubsystem =
		GameInstance->GetSubsystem<UGoHomeSaveSubsystem>();

	if (!SaveSubsystem)
	{
		OutResult.Result = EItemShopResult::ShopUnavailable;
		return false;
	}

	// 상품, 장소, 가격, 코인을 먼저 확인한다.
	int32 TotalPrice = 0;
	int32 CurrentFunds = 0;

	// 검증 함수 전용 결과 변수
	EItemShopResult ValidationResult =
		EItemShopResult::ShopUnavailable;

	if (!TryValidatePurchase(
		Request,
		TotalPrice,
		CurrentFunds,
		ValidationResult))
	{
		// 검증 결과를 구매 결과 구조체에 옮긴다.
		OutResult.Result = ValidationResult;
		OutResult.RemainingFunds = CurrentFunds;
		return false;
	}

	const FString OwnerPlayerKey =
		MakeShopOwnerPlayerKey(Requester);

	if (OwnerPlayerKey.IsEmpty())
	{
		OutResult.Result = EItemShopResult::InvalidRequester;
		OutResult.Message =
			FText::FromString(TEXT("플레이어 ID를 찾을 수 없습니다."));
		return false;
	}

	// 상품별 이번 구매 수량을 합친다.
	TMap<FName, int32> RequestedByProduct;

	int32 RequestedItemCount = 0;

	for (const FItemShopCartLine& Line : Request.Lines)
	{
		// 현재는 다른 플레이어에게 선물하기를 허용하지 않는다.
		if (!Line.TargetPlayerKey.IsEmpty())
		{
			OutResult.Result = EItemShopResult::InvalidTarget;
			OutResult.Message =
				FText::FromString(TEXT("현재는 자기 자신에게만 지급할 수 있습니다."));
			return false;
		}

		RequestedByProduct.FindOrAdd(Line.ProductId)
			+= Line.Quantity;

		RequestedItemCount += Line.Quantity;
	}

	// 상품별 구매 횟수 확인.
	for (const TPair<FName, int32>& Pair :
		RequestedByProduct)
	{
		const FItemShopProduct* Product =
			FindProduct(Pair.Key);

		if (!Product)
		{
			OutResult.Result = EItemShopResult::InvalidProduct;
			return false;
		}

		const int32 AlreadyPurchased =
			SaveSubsystem->GetShopPurchasedQuantity(
				OwnerPlayerKey,
				Product->ProductId);

		const int32 RequestedQuantity = Pair.Value;

		if (Product->MaxPurchasesPerRun < 0 ||
			AlreadyPurchased >
			Product->MaxPurchasesPerRun - RequestedQuantity)
		{
			OutResult.Result =
				EItemShopResult::PurchaseLimitReached;

			OutResult.RemainingFunds = CurrentFunds;
			OutResult.Message =
				FText::FromString(
					TEXT("이번 런의 구매 한도를 초과했습니다."));

			return false;
		}

		// 하나만 가질 수 있는 상품인지 확인.
		if (Product->bUniquePerPlayer &&
			SaveSubsystem->GetShopOwnedQuantity(
				OwnerPlayerKey,
				Product->ProductId) > 0)
		{
			OutResult.Result =
				EItemShopResult::PurchaseLimitReached;

			OutResult.RemainingFunds = CurrentFunds;
			OutResult.Message =
				FText::FromString(
					TEXT("이미 보유한 상품입니다."));

			return false;
		}
	}

	// 인벤토리 빈칸 확인.
	int32 EmptySlotCount = 0;

	for (int32 SlotIndex = 0;
		SlotIndex < Inventory->GetInventorySlotCount();
		++SlotIndex)
	{
		if (!Inventory->GetItemInSlot(SlotIndex))
		{
			++EmptySlotCount;
		}
	}

	if (RequestedItemCount > EmptySlotCount)
	{
		OutResult.Result = EItemShopResult::InventoryFull;
		OutResult.RemainingFunds = CurrentFunds;
		OutResult.Message =
			FText::FromString(TEXT("인벤토리 공간이 부족합니다."));
		return false;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		OutResult.Result = EItemShopResult::SpawnFailed;
		return false;
	}

	TArray<AItemActorBase*> GrantedItems;

	// 중간 실패 시 이미 만든 아이템을 되돌린다.
	auto RollbackItems = [&]()
		{
			for (AItemActorBase* Item : GrantedItems)
			{
				if (!Item)
				{
					continue;
				}

				UnregisterRuntimeShopItem(Item);
				Item->ServerDrop();
				Item->Destroy();
			}

			GrantedItems.Reset();
		};

	// 상품별로 실제 아이템을 생성한다.
	for (const FItemShopCartLine& Line : Request.Lines)
	{
		const FItemShopProduct* Product =
			FindProduct(Line.ProductId);

		if (!Product)
		{
			RollbackItems();

			OutResult.Result =
				EItemShopResult::InvalidProduct;

			return false;
		}

		for (int32 Index = 0;
			Index < Line.Quantity;
			++Index)
		{
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(
				Pawn->GetActorLocation());
			SpawnTransform.SetRotation(
				Pawn->GetActorQuat());

			AItemActorBase* SpawnedItem =
				World->SpawnActorDeferred<AItemActorBase>(
					Product->ActorClass,
					SpawnTransform,
					Requester,
					Pawn,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (!SpawnedItem)
			{
				RollbackItems();

				OutResult.Result =
					EItemShopResult::SpawnFailed;

				return false;
			}

			// 생성 전에 상품 데이터를 넣는다.
			SpawnedItem->ItemData =
				Product->ItemData;

			SpawnedItem->FinishSpawning(
				SpawnTransform);

			// 기존 인벤토리 지급 방식을 그대로 사용한다.
			SpawnedItem->OnInteract(Pawn);

			if (Inventory->FindSlotIndexOf(
				SpawnedItem) == INDEX_NONE)
			{
				SpawnedItem->Destroy();
				RollbackItems();

				OutResult.Result =
					EItemShopResult::InventoryFull;

				return false;
			}

			// 아이템 코어를 수정하지 않고 상점 장부에만 등록한다.
			RegisterRuntimeShopItem(
				SpawnedItem,
				Inventory,
				Product->ProductId,
				OwnerPlayerKey);

			GrantedItems.Add(SpawnedItem);
		}
	}

	// 모든 아이템 지급이 성공한 후 코인을 차감한다.
	if (!SaveSubsystem->TrySpendFunds(TotalPrice))
	{
		RollbackItems();

		OutResult.Result =
			EItemShopResult::NotEnoughFunds;

		return false;
	}

	// 구매 기록을 저장한다.
	for (const TPair<FName, int32>& Pair :
		RequestedByProduct)
	{
		SaveSubsystem->AddShopPurchase(
			OwnerPlayerKey,
			Pair.Key,
			Pair.Value);
	}

	// 인게임 HUD에 최신 코인을 전달한다.
	if (AExplorationGameState* GameState =
		World->GetGameState<AExplorationGameState>())
	{
		GameState->SetCurrentFunds(
			SaveSubsystem->GetCurrentFunds());
	}

	OutResult.Result = EItemShopResult::Success;
	OutResult.RemainingFunds =
		SaveSubsystem->GetCurrentFunds();

	OutResult.Message =
		FText::FromString(TEXT("구매가 완료되었습니다."));

	return true;
}