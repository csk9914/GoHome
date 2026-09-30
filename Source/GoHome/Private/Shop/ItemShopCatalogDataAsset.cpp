
#include "Shop/ItemShopCatalogDataAsset.h"




const FItemShopProduct* UItemShopCatalogDataAsset::FindProduct(
	FName ProductId
) const
{
	return Products.FindByPredicate(
		[ProductId](const FItemShopProduct& Product)
		{
			return Product.ProductId == ProductId;
		}
	);
}