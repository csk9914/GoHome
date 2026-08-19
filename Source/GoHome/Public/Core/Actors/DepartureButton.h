// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "DepartureButton.generated.h"

UCLASS()
class GOHOME_API ADepartureButton : public AActor, public IInteractable    
{
	GENERATED_BODY()

public:
	ADepartureButton();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	
};
