//THE

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableTargetChanged, AActor*, NewTarget);

// 카메라 정면 트레이스로 상호작용 대상을 탐지한다(로컬 컨트롤 폰(Pawn)에서만 동작).
// 대상 변경은 OnInteractableTargetChanged로 브로드캐스트(HUD 프롬프트 표시/숨김용).

UCLASS( ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent) )
class GOHOME_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UInteractionComponent();

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableTargetChanged OnInteractableTargetChanged;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(Server, Reliable)
	void Server_RequestInteract(AActor* Target);

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceDistance = 200.f;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceInterval = 0.1f;


	// 지금 조준 중인 대상 (없으면 nullptr). HUD/디버깅에서 조회용.
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void PerformTrace();

	UPROPERTY()
	TObjectPtr<UCameraComponent> CachedCamera;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	float TimeSinceLastTrace = 0.f;
	
};
