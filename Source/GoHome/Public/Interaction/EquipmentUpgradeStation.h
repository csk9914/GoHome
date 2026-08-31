#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "EquipmentUpgradeStation.generated.h"

class UStaticMeshComponent;

UCLASS()
class GOHOME_API AEquipmentUpgradeStation : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AEquipmentUpgradeStation();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Equipment Upgrade")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};