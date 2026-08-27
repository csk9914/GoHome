

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OxygenDrainBoostConfig.generated.h"


USTRUCT(BlueprintType)
struct FOxygenDrainBoostRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float DrainMultiplier = 1.0f;
};

UCLASS(BlueprintType)
class GOHOME_API UOxygenDrainBoostConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    float GetDrainMultiplierForInstigator(const AActor* InstigatorActor) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen")
    FOxygenDrainBoostRule DefaultRule;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen")
    TMap<TSubclassOf<AActor>, FOxygenDrainBoostRule> MonsterRules;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float MaxMonsterDrainMultiplier = 3.0f;
	
};
