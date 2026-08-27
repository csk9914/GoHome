

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OxygenDrainBoostLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GOHOME_API UOxygenDrainBoostLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Oxygen")
	static void StartOxygenDrainBoostFromInstigator(AActor* TargetActor, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Oxygen")
	static void StopOxygenDrainBoostFromInstigator(AActor* TargetActor, AActor* InstigatorActor);
	
};
