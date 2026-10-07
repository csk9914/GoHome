#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Item/ItemSpawnTypes.h"
#include "RecoveryItemSpawnTypes.generated.h"

class ARecoveryItemBase;

USTRUCT(BlueprintType)
struct GOHOME_API FRecoveryItemSpawnWeightRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recovery Spawn", meta = (ToolTip = "아이템 스폰 지점과 같은 구역을 선택합니다."))
	ESpawnDangerTier Tier = ESpawnDangerTier::Far;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recovery Spawn", meta = (ToolTip = "해당 구역에서 생성할 회복 아이템 블루프린트 클래스입니다."))
	TSubclassOf<ARecoveryItemBase> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recovery Spawn", meta = (ClampMin = "0.0", ToolTip = "같은 구역 안에서 이 아이템이 선택될 상대 확률입니다. 0이면 선택되지 않습니다."))
	float Weight = 0.f;
};
