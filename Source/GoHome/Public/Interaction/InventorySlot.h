

#pragma once

#include "CoreMinimal.h"
#include "InventorySlot.generated.h"

class AItemActorBase;

/**
 * 인벤토리 1슬롯 데이터. UInventoryComponent가 4개 배열로 보유하며, 슬롯별로 개별 RepNotify된다.
 */
USTRUCT(BlueprintType)
struct GOHOME_API FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<AItemActorBase> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 0;
};
