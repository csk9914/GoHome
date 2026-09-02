// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ExpeditionZoneDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GOHOME_API UExpeditionZoneDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zone")
	FName ZoneId;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zone")
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zone")
	TSoftObjectPtr<UWorld> MapPath;
	
	// 해당 맵의 할당량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zone")
	int32 MapQuota = 0;

	// 탐사 제한 시간(초). 0 이하면 시간 제한 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zone")
	float TimeLimitSeconds = 0.f;

};
