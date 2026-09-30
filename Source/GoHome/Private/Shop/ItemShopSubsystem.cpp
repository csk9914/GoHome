#include "Shop/ItemShopSubsystem.h"
#include "Shop/ItemShopCatalogDataAsset.h"
#include "Shop/ItemShopTypes.h"
#include "UObject/UObjectGlobals.h"
#include "Core/GoHomeGameState.h"
#include "Core/ExpeditionState.h"
#include "Save/GoHomeSaveSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
	const TCHAR* ItemShopCatalogPath =
		TEXT("/Game/GoHome/Developers/LSA/Data/DA_ItemShopCatalog.DA_ItemShopCatalog");
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