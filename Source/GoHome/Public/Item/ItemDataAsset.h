

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

/** 아이템 종별 무게/가치/파손·소음 플래그. AItemActorBase가 참조해 종을 구분한다. */
UCLASS(BlueprintType)
class GOHOME_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float Weight = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float Value = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bCanBreak = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bMakesNoise = false;

	// 파손형 전용(bCanBreak일 때만 의미 있음).
	// 01문서 4절 수치가 기본값 - 감산형: 파손마다 -30%p, 40%에서 하한.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",
		meta = (ToolTip = "낙하/충돌 속도가 이 값(cm/s) 이상이면 파손 판정"))
	float BreakVelocityThreshold = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",
		meta = (ToolTip = "파손 1회당 잔존가치 비율에서 차감되는 값 (0.3 = -30%p, 감산형)", 
			ClampMin = "0.0", ClampMax = "1.0"))
	float BreakValuePenaltyPercent = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",
		meta = (ToolTip = "최대 파손 횟수",
			ClampMin = "0"))
	int32 MaxBreakCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",
		meta = (ToolTip = "잔존가치 하한 비율 (0.4 = 40%)",
			ClampMin = "0.0", ClampMax = "1.0"))
	float MinValuePercent = 0.4f;

};
