// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "ZoneSelectMonitor.generated.h"

UCLASS()
class GOHOME_API AZoneSelectMonitor : public AActor, public IInteractable     
{
	GENERATED_BODY()

public:
	AZoneSelectMonitor();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
};
