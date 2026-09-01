// 

#pragma once

#include "CoreMinimal.h"
#include "GoHomeGameState.h"
#include "ExplorationGameState.generated.h"

class UHealthComponent;

/**
 * 
 */

UCLASS()
class GOHOME_API AExplorationGameState : public AGoHomeGameState
{
	GENERATED_BODY()

public:
	AExplorationGameState();

protected:
	virtual void BeginPlay() override;

	
private:

	
};
