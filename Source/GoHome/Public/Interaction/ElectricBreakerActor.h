

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "ElectricBreakerActor.generated.h"

class UStaticMeshComponent;
class AElectricSwitchboardActor;

// 배전반과 짝지어 배치하는 원거리 차단기.
// 기존 E키 상호작용 파이프라인(IInteractable) 그대로 재사용.
UCLASS()
class GOHOME_API AElectricBreakerActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	

	AElectricBreakerActor();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Breaker")
	TObjectPtr<UStaticMeshComponent> BreakerMesh;

	// 레벨 디자이너가 배치 시 연결 (BreakableWallActor::PassageNavLink와 동일 관용구 - 같은 방/다른 방 무관).
	UPROPERTY(EditInstanceOnly, Category = "Breaker")
	TObjectPtr<AElectricSwitchboardActor> LinkedSwitchboard;


};
