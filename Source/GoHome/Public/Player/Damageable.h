

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Damageable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 데미지를 받을 수 있는 오브젝트가 구현한다 (UHealthComponent 소유 액터 등).
 * AI 공격, Player 자신의 질식 데미지가 이 인터페이스로만 호출한다 (대상 타입 결합 금지).
 * BlueprintNativeEvent: BP_Monster가 공격 시 Execute_ApplyDamage로 호출할 수 있어야 한다.
 */
class GOHOME_API IDamageable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
	void ApplyDamage(float Amount, AActor* Instigator, FName DamageType);
};
