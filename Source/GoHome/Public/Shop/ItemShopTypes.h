#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActorBase.h"
#include "ItemShopTypes.generated.h"

class UItemDataAsset;

// 공용 데이터 구조
UENUM(BlueprintType)
enum class EItemShopContext : uint8
{
	LobbyLoadout UMETA(DisplayName = "Lobby Loadout"),
	InGameImmediate UMETA(DisplayName = "In Game Immediate")
};

UENUM(BlueprintType)
enum class EItemShopCategory : uint8
{
	Tools UMETA(DisplayName = "Tools"),
	Survival UMETA(DisplayName = "Survival"),
	Gear UMETA(DisplayName = "Gear"),
	Defense UMETA(DisplayName = "Defense")
};

UENUM(BlueprintType)
enum class EItemShopResult : uint8
{
	Success,
	ShopUnavailable,
	InvalidRequester,
	InvalidProduct,
	InvalidQuantity,
	InvalidTarget,
	NotEnoughFunds,
	InventoryFull,
	Overweight,
	NotAllowedInContext,
	SpawnFailed,
	PurchaseLimitReached
};

// 상품 하나의 구조
USTRUCT(BlueprintType)
struct GOHOME_API FItemShopProduct
{
	GENERATED_BODY()

public:
	// 아이템 고유 이름 (ex. Hammer)
	// 서버와 UI가 서로 아이템을 찾을 때 사용 (닉네임이나 화면 표시 이름을 ID로 사용 x)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	FName ProductId = NAME_None;

	// 기존 아이템 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UItemDataAsset> ItemData = nullptr;

	// 실제로 생성할 아이템BP (Hammer -> BP_Hammer)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<AItemActorBase> ActorClass;

	// 상점 구매 가격 (ItemData.Value 와 다름)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	int32 PurchasePrice = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	EItemShopCategory Category = EItemShopCategory::Tools;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	bool bCanBuyInLobby = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	bool bCanBuyInGame = true;

	// 한번에 몇 개까지 살 수 있는지 (2개 사면 인벤토리 2칸 차지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	int32 MaxQuantityPerOrder = 1;

	// 한 런 동안 구매할 수 있는 총 수량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	int32 MaxPurchasesPerRun = 3;

	// 플레이어 한 명당 하나만 가질 수 있는지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	bool bUniquePerPlayer = false;
};

// 장바구니 구조
USTRUCT(BlueprintType)
struct GOHOME_API FItemShopCartLine
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	FName ProductId = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	int32 Quantity = 1;

	// 닉네임이 아니라 SteamID 문자열을 저장합니다.
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	FString TargetPlayerKey;
};

// 구매요청 구조 (UI -> 서버 요청)
USTRUCT(BlueprintType)
struct GOHOME_API FItemShopPurchaseRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	TArray<FItemShopCartLine> Lines;

	// 현재 상점은 탐사 중 즉시 구매만 사용한다.
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	EItemShopContext Context = EItemShopContext::InGameImmediate;
};


// 상점 상품의 구매 및 현재 보유 상태
USTRUCT(BlueprintType)
struct GOHOME_API FItemShopLoadoutEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	FString OwnerPlayerKey;

	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	FName ProductId = NAME_None;

	// 지금까지 구매한 총 수량
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	int32 PurchasedQuantity = 0;

	// 현재 실제로 보유한 수량
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	int32 OwnedQuantity = 0;
};

// 구매 결과 구조 (서버가 UI에게 보냄)
USTRUCT(BlueprintType)
struct GOHOME_API FItemShopPurchaseResult
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	EItemShopResult Result = EItemShopResult::ShopUnavailable;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 RemainingFunds = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FText Message;
};

