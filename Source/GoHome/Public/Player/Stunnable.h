#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Stunnable.generated.h"


UINTERFACE(MinimalAPI, BlueprintType)
class UStunnable : public UInterface
{
	GENERATED_BODY()
};

// 스턴 + 넉백을 받을 수 있는 오브젝트가 구현한다(AGoHomeCharacter 등).
// 환경 위해요소(합선 배전반 등)가 이 인터페이스로만 호출한다.
// (대상 타입 결합 금지 -> IDamageable과 같은 원칙).
// BlueprintNativeEvent : BP 쪽 위해요소도 Execute_ApplyStun으로 호출할 수 있어야 함.


class GOHOME_API IStunnable
{
	GENERATED_BODY()


public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Stun")
	void ApplyStun(float Duration, FVector KnockbackImpulse, AActor* Instigator);
};
