//THE

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemSpawnTypes.generated.h"


class UItemDataAsset;

UENUM(BlueprintType)
enum class ESpawnDangerTier : uint8
{
	Near,  // 초입
	Mid,  // 중간
	Far   // 후반
};

USTRUCT(BlueprintType)
struct GOHOME_API FItemSpawnWeightRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Spawn")
	ESpawnDangerTier Tier = ESpawnDangerTier::Near;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TObjectPtr<UItemDataAsset> ItemData;

	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};