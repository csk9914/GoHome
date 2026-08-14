

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class APawn;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 가능한 오브젝트가 구현한다 (AItemActorBase 등).
 */
class GOHOME_API IInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(APawn* InstigatorPawn) const = 0;
	virtual void OnInteract(APawn* InstigatorPawn) = 0;

	// 상호 작용 프롬프트 UI에 표시할 문구. 필요할 구현체만 오버라이드, 기본값은 범용 텍스트.
	// BlueprintNativeEvent인 이유: 위젯 쪽에서 AActor* 타입(구체 클래스 모름)에 대고
	// 바로 호출해야 하는데, 일반 C++ 가상함수는 Blueprint에서 그렇게 못 부름.

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	FText GetInteractionPromptText() const;
};
