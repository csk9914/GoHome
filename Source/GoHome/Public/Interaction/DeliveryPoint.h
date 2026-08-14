//THE

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "DeliveryPoint.generated.h"

class UStaticMeshComponent;


// 납품 지점: 상호작용 시 인벤토리 전체를 정산하고 제거한다(01/02문서 4절).

UCLASS()
class GOHOME_API ADeliveryPoint : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	

	ADeliveryPoint();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Delivery")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
