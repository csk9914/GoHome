

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Shop/ItemShopTypes.h"
#include "ItemShopSubsystem.generated.h"


class UItemShopCatalogDataAsset;
class AItemActorBase;
class UInventoryComponent;
class APlayerController;


// 현재 라운드에서 상점이 만든 아이템 기록
struct FItemShopRuntimeItem
{
	TWeakObjectPtr<AItemActorBase> Item;
	TWeakObjectPtr<UInventoryComponent> OwnerInventory;

	FName ProductId = NAME_None;
	FString OwnerPlayerKey;
};


UCLASS()
class GOHOME_API UItemShopSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& CollectionBase) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Item Shop")
	bool IsShopAvailable() const
	{
		return bShopAvailable;
	}

	UFUNCTION(BlueprintPure, Category = "Item Shop")
	UItemShopCatalogDataAsset* GetCatalog() const
	{
		return Catalog;
	}

	const FItemShopProduct* FindProduct(FName ProductId) const;

	// 서버 검증
	bool TryCalculateOrderTotal(
		const FItemShopPurchaseRequest& Request,
		int32& OutTotalPrice,
		EItemShopResult& OutResult
	) const;

	// 구매 가능 여부와 현재 보유 코인을 확인한다.
	// 아직 실제 코인 차감은 하지 않는다.
	bool TryValidatePurchase(
		const FItemShopPurchaseRequest& Request,
		int32& OutTotalPrice,
		int32& OutCurrentFunds,
		EItemShopResult& OutResult
	) const;


	// 상점에서 만든 아이템을 장부에 등록한다.
	void RegisterRuntimeShopItem(
		AItemActorBase* Item,
		UInventoryComponent* OwnerInventory,
		FName ProductId,
		const FString& OwnerPlayerKey);

	// 구매 처리 중 실패했을 때 장부에서 제거한다.
	void UnregisterRuntimeShopItem(AItemActorBase* Item);

	// 구매 검증부터 아이템 지급, 코인 차감까지 처리한다.
	bool TryProcessPurchase(
		APlayerController* Requester,
		const FItemShopPurchaseRequest& Request,
		FItemShopPurchaseResult& OutResult);

	// 다음 탐사 라운드 시작 시 저장된 보유 아이템을 지급한다.
	bool TryGrantSavedLoadout(APlayerController* Requester);

	// 라운드가 완전히 끝났을 때 런타임 장부를 비운다.
	void ClearRuntimeShopItems();

	// 라운드 종료 직전, 실제 인벤토리에 남아 있는 상점 아이템 수를 저장한다.
	void ReconcileRuntimeShopItems();


private:
	UPROPERTY()
	TObjectPtr<UItemShopCatalogDataAsset> Catalog;

	bool bShopAvailable = false;

	// 현재 라운드의 상점 아이템 장부
	TArray<FItemShopRuntimeItem> RuntimeShopItems;

	// 같은 라운드에서 중복 지급하지 않기 위한 플레이어 기록
	TSet<FString> LoadoutGrantedPlayers;
};
