//THE

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/ItemSpawnTypes.h"
#include "ItemSpawnManager.generated.h"

class UDataTable;
class UItemDataAsset;

UCLASS()
class GOHOME_API AItemSpawnManager : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AItemSpawnManager();

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TObjectPtr<UDataTable> SpawnWeightTable;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TMap<ESpawnDangerTier, int32> TargetCountPerTier;

protected:

	virtual void BeginPlay() override;

private:

	void SpawnItems();
	UItemDataAsset* PickWeightedRandomItemData(ESpawnDangerTier Tier) const;

};
