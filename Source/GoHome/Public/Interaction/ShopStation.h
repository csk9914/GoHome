

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "ShopStation.generated.h"


class APawn;
class UStaticMeshComponent;


UCLASS()
class GOHOME_API AShopStation : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:
	AShopStation();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

};
