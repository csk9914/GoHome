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

private:
	void ExecuteDelayedTravel(FString TravelPath);
	
protected:
	
	// BP_Submarine의 Timeline_Door 길이(현재 5.0초), + 여유 값으로 3초 추가
	UPROPERTY(EditDefaultsOnly, Category = "DockingDoor")
	float DoorCloseDelay = 8.f;
	
private:
	FTimerHandle TravelDelayTimer;
	
};
