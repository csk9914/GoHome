

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/Damageable.h"
#include "HealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathDynamic);

/**
 * IDamageable을 구현해 "누가 데미지를 주는지" 몰라도 되게 한다.
 * HP 0 도달 시 서버 전용 OnDeath를 1회만 브로드캐스트한다 (bIsDead로 중복 방지).
 */
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class GOHOME_API UHealthComponent : public UActorComponent, public IDamageable
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Health")
	float MaxHP = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_HP, BlueprintReadOnly, Category = "Health")
	float HP = 100.f;

	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeathDynamic OnDeathDynamic;

	virtual void ApplyDamage_Implementation(float Amount, AActor* Instigator, FName DamageType) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_HP();

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();
};
