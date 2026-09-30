

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Shop/ItemShopTypes.h"
#include "ItemShopCatalogDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GOHOME_API UItemShopCatalogDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TArray<FItemShopProduct> Products;

	const FItemShopProduct* FindProduct(FName ProductId) const;

	UFUNCTION(BlueprintPure, Category = "Shop")
	TArray<FItemShopProduct> GetProducts() const
	{
		return Products;
	}
};
