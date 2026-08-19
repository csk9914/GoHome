

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MultiActorGateComponent.generated.h"

class ACoopLeverActor;
class UDockingDoorComponent;

// 레버 2개가 동시에 Active인지 서버가 판정해서, 같은 액터에 붙은 UDockingDoorComponent를 여닫는다.
// 도킹 문 컴포넌트를 새로 만들지 않고 그대로 재사용(01문서 7-1절).

UCLASS( ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent) )
class GOHOME_API UMultiActorGateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UMultiActorGateComponent();

	// 감지할 레버 2개. 레벨/BP에서 지정.
	UPROPERTY(EditAnywhere, Category = "Gate")
	TObjectPtr<ACoopLeverActor> LeverA;

	UPROPERTY(EditAnywhere, Category = "Gate")
	TObjectPtr<ACoopLeverActor> LeverB;

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void OnLeverStateChanged(bool bNewActive);

	void EvaluateGateState();

	// 같은 액터에 붙어있는 도어 컴포넌트를 자동으로 찾아 캐시.
	UPROPERTY()
	TObjectPtr<UDockingDoorComponent> CachedDoor;
};
