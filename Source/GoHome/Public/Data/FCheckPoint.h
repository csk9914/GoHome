#pragma once
#include "CoreMinimal.h"
#include "FCheckPoint.generated.h"

// 자금 관문 1개 — 판정 라운드와 목표 할당액. 전체 스케줄은 UEconomyConfigDataAsset::CheckPoints.
USTRUCT(BlueprintType)
struct FCheckPoint
{
	GENERATED_BODY()

	// 판정 라운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CheckPoint")
	int32 Round = 0;

	// 목표 할당액
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CheckPoint")
	int32 TargetQuota = 0;
};
