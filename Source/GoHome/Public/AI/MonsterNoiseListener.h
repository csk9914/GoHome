

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AI/NoiseType.h"
#include "MonsterNoiseListener.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UMonsterNoiseListener : public UInterface
{
	GENERATED_BODY()
};

/**
 * GenerateNoise가 반경 내 각 몬스터에 동기 호출한다. "무엇으로 전이할지"는 구현체(BP_Monster) 내부가 정한다
 * (경계는 호출 계약과 시그니처까지만).
 */
class GOHOME_API IMonsterNoiseListener
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Noise")
	void OnNoiseHeard(FVector Location, float Radius, ENoiseType Type, AActor* Source);
};
