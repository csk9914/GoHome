//THE

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/CoopCarryObjectBase.h"
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

	UFUNCTION(Server, Reliable)
	void Server_RequestDeliverCarry();

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceDistance = 200.f;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float TraceInterval = 0.1f;

	// 오브젝트 테두리 강조 표시용. Custom Depth 스텐실 값.
	// 추후 아이템 종류에 따라 색상을 다르게 쓰고 싶다면, 해당 값을 종류별로 다르게 주고,
	// 포스트프로세싱 머터리얼에서 값별로 분기하면 됨(지금은 1가지만 사용).
	UPROPERTY(EditAnywhere, Category = "Interaction")
	int32 OutlineStencilValue = 1;

	// 지금 조준 중인 대상 (없으면 nullptr). HUD/디버깅에서 조회용.
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	// NewTarget이 IInteractable을 구현하면 그 프롬프트 문구를 그 외에는 기본 문구를 반환함.
	// 블루프린트에서 인터페이스 "메시지" 노드를 직접 찾기 애매할 때 쓰라고 만든 함수.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static FText GetInteractionPromptTextFor(AActor* Target);


protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void PerformTrace();

	// Target의 모든 PrimitiveComponent에 Custom Depth 렌더링을 켜거나 끔(메쉬 테두리 강조 표시용).
	void SetOutlineEnabled(AActor* Target, bool bEnabled);

	UPROPERTY()
	TObjectPtr<UCameraComponent> CachedCamera;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	float TimeSinceLastTrace = 0.f;
	
};
