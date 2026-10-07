#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/ItemSpawnTypes.h"
#include "RecoveryItemSpawnPoint.generated.h"

UCLASS()
class GOHOME_API ARecoveryItemSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ARecoveryItemSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recovery Spawn", meta = (ToolTip = "이 회복 스폰 지점이 속하는 구역입니다."))
	ESpawnDangerTier DangerTier = ESpawnDangerTier::Far;
};
