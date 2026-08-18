
#include "Item/ItemSpawnManager.h"
#include "Item/ItemSpawnPoint.h"
#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"
#include "EngineUtils.h"


AItemSpawnManager::AItemSpawnManager()
{
 	
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;   // 클라에서 Role이 제대로 SimulatedProxy로 내려감

	TargetCountPerTier.Add(ESpawnDangerTier::Near, 4);
	TargetCountPerTier.Add(ESpawnDangerTier::Mid, 5);
	TargetCountPerTier.Add(ESpawnDangerTier::Far, 6);

}


void AItemSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;
	SpawnItems();
}


void AItemSpawnManager::SpawnItems()
{
	if (!SpawnWeightTable) return;

	TMap<ESpawnDangerTier, TArray<AItemSpawnPoint*>> PointsByTier;
	for (TActorIterator<AItemSpawnPoint> It(GetWorld()); It; ++It)
	{
		PointsByTier.FindOrAdd(It->DangerTier).Add(*It);
	}

	for (const TPair<ESpawnDangerTier, int32>& CountPair : TargetCountPerTier)
	{
		const ESpawnDangerTier Tier = CountPair.Key;
		const int32 TargetCount = CountPair.Value;

		TArray<AItemSpawnPoint*>* PointsPtr = PointsByTier.Find(Tier);
		if (!PointsPtr || PointsPtr->Num() == 0) continue;

		TArray<AItemSpawnPoint*> Points = *PointsPtr;

		for (int32 i = Points.Num() - 1; i > 0; --i)
		{
			const int32 j = FMath::RandRange(0, i);
			Points.Swap(i, j);
		}

		const int32 ActualCount = FMath::Min(TargetCount, Points.Num());

		for (int32 i = 0; i < ActualCount; ++i)
		{
			UItemDataAsset* SelectedItemData = PickWeightedRandomItemData(Tier);
			if (!SelectedItemData) continue;

			const FTransform SpawnTransform = Points[i]->GetActorTransform();

			AItemActorBase* SpawnedItem = GetWorld()->SpawnActorDeferred<AItemActorBase>(
				AItemActorBase::StaticClass(), SpawnTransform);

			if (SpawnedItem)
			{
				SpawnedItem->ItemData = SelectedItemData;
				SpawnedItem->FinishSpawning(SpawnTransform);
				SpawnedItem->ForceNetUpdate();   
			}
		}
	}
}

UItemDataAsset* AItemSpawnManager::PickWeightedRandomItemData(ESpawnDangerTier Tier) const
{
	TArray<FItemSpawnWeightRow*> AllRows;
	SpawnWeightTable->GetAllRows<FItemSpawnWeightRow>(TEXT("PickWeightedRandomItemData"), AllRows);

	TArray<FItemSpawnWeightRow*> TierRows;
	float TotalWeight = 0.f;
	for (FItemSpawnWeightRow* Row : AllRows)
	{
		if (Row && Row->Tier == Tier && Row->ItemData)
		{
			TierRows.Add(Row);
			TotalWeight += Row->Weight;
		}
	}

	if (TierRows.Num() == 0 || TotalWeight <= 0.f) return nullptr;

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (FItemSpawnWeightRow* Row : TierRows)
	{
		Roll -= Row->Weight;
		if (Roll <= 0.f)
		{
			return Row->ItemData;
		}
	}
	return TierRows.Last()->ItemData;
}