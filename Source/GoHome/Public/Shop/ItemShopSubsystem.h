

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Shop/ItemShopTypes.h"
#include "ItemShopSubsystem.generated.h"


class UItemShopCatalogDataAsset;


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

private:
	UPROPERTY()
	TObjectPtr<UItemShopCatalogDataAsset> Catalog;

	bool bShopAvailable = false;
};
