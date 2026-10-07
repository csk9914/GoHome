#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/ItemSpawnTypes.h"
#include "RecoveryItemSpawnManager.generated.h"

class UDataTable;

UCLASS()
class GOHOME_API ARecoveryItemSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	ARecoveryItemSpawnManager();

	UPROPERTY(EditAnywhere, Category = "Recovery Spawn", meta = (ToolTip = "구역별 회복 아이템 종류와 등장 가중치를 담는 전용 데이터 테이블입니다."))
	TObjectPtr<UDataTable> RecoverySpawnWeightTable;

	// 미설정 상태에서는 스폰하지 않는다. 실제 개수는 맵별로 설정한다.
	UPROPERTY(EditAnywhere, Category = "Recovery Spawn", meta = (ToolTip = "각 구역에 배치할 회복 아이템 개수입니다. 비워 두면 스폰하지 않습니다."))
	TMap<ESpawnDangerTier, int32> TargetCountPerTier;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnRecoveryItems();
	TSubclassOf<class ARecoveryItemBase> PickWeightedItemClass(ESpawnDangerTier Tier) const;
};
