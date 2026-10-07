#include "Item/RecoveryItemSpawnManager.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Item/RecoveryItemBase.h"
#include "Item/RecoveryItemSpawnPoint.h"
#include "Item/RecoveryItemSpawnTypes.h"

ARecoveryItemSpawnManager::ARecoveryItemSpawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARecoveryItemSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnRecoveryItems();
	}
}

void ARecoveryItemSpawnManager::SpawnRecoveryItems()
{
	if (!RecoverySpawnWeightTable)
	{
		return;
	}

	TMap<ESpawnDangerTier, TArray<ARecoveryItemSpawnPoint*>> PointsByTier;
	for (TActorIterator<ARecoveryItemSpawnPoint> It(GetWorld()); It; ++It)
	{
		if (ARecoveryItemSpawnPoint* Point = *It)
		{
			PointsByTier.FindOrAdd(Point->DangerTier).Add(Point);
		}
	}

	for (const TPair<ESpawnDangerTier, int32>& CountPair : TargetCountPerTier)
	{
		const int32 TargetCount = FMath::Max(0, CountPair.Value);
		TArray<ARecoveryItemSpawnPoint*>* Points = PointsByTier.Find(CountPair.Key);
		if (TargetCount == 0 || !Points || Points->IsEmpty())
		{
			continue;
		}

		for (int32 Index = Points->Num() - 1; Index > 0; --Index)
		{
			const int32 SwapIndex = FMath::RandRange(0, Index);
			Points->Swap(Index, SwapIndex);
		}
		const int32 SpawnCount = FMath::Min(TargetCount, Points->Num());
		for (int32 Index = 0; Index < SpawnCount; ++Index)
		{
			ARecoveryItemSpawnPoint* Point = (*Points)[Index];
			TSubclassOf<ARecoveryItemBase> ItemClass = PickWeightedItemClass(CountPair.Key);
			if (!Point || !ItemClass)
			{
				continue;
			}

			const FTransform SpawnTransform = Point->GetActorTransform();
			ARecoveryItemBase* SpawnedItem = GetWorld()->SpawnActorDeferred<ARecoveryItemBase>(
				ItemClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

			if (SpawnedItem)
			{
				SpawnedItem->FinishSpawning(SpawnTransform);
				SpawnedItem->ForceNetUpdate();
			}
		}
	}
}

TSubclassOf<ARecoveryItemBase> ARecoveryItemSpawnManager::PickWeightedItemClass(ESpawnDangerTier Tier) const
{
	if (!RecoverySpawnWeightTable)
	{
		return nullptr;
	}

	TArray<FRecoveryItemSpawnWeightRow*> AllRows;
	RecoverySpawnWeightTable->GetAllRows<FRecoveryItemSpawnWeightRow>(
		TEXT("PickWeightedRecoveryItemClass"),
		AllRows);

	TArray<const FRecoveryItemSpawnWeightRow*> EligibleRows;
	float TotalWeight = 0.f;
	for (const FRecoveryItemSpawnWeightRow* Row : AllRows)
	{
		if (Row && Row->Tier == Tier && Row->ItemClass && Row->Weight > 0.f)
		{
			EligibleRows.Add(Row);
			TotalWeight += Row->Weight;
		}
	}

	if (EligibleRows.IsEmpty() || TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FRecoveryItemSpawnWeightRow* Row : EligibleRows)
	{
		Roll -= Row->Weight;
		if (Roll <= 0.f)
		{
			return Row->ItemClass;
		}
	}

	return EligibleRows.Last()->ItemClass;
}
