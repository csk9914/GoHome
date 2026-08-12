

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/Damageable.h"
#include "Player/DeathNotifier.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathDynamic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, CurrentHP, float, MaxHP);

/**
 * 플레이어 HP를 담당하는 컴포넌트입니다.
 *
 * 몬스터, 산소, 함정 같은 외부 시스템은 플레이어 클래스를 직접 알 필요 없이
 * IDamageable::ApplyDamage만 호출합니다. HP 변경은 서버에서만 처리하고,
 * 클라이언트와 UI는 복제/델리게이트를 통해 결과만 받아서 표시합니다.
 */
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class GOHOME_API UHealthComponent : public UActorComponent, public IDamageable, public IDeathNotifier
{
	GENERATED_BODY()

public:
	UHealthComponent();

	// HP UI는 이 이벤트를 구독해서 텍스트를 갱신합니다.
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHPChanged OnHPChanged;


	// GameState 같은 C++ 시스템이 서버에서 사망자를 집계할 때 사용합니다.
	FOnDeath OnDeath;

	FSimpleMulticastDelegate OnDeath;

	// 블루프린트에서 사망 연출, UI, 사운드 등을 연결할 때 사용합니다.
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeathDynamic OnDeathDynamic;

	virtual void ApplyDamage_Implementation(float Amount, AActor* Instigator, FName DamageType) override;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHP() const { return HP; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	virtual FSimpleMulticastDelegate& GetOnDeathDelegate() override { return OnDeath; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	// 에디터/BP에서 최대 HP만 조절하고, 실제 HP 변경은 ApplyDamage 경로로만 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Health")
	float MaxHP = 100.f;

	// 서버가 바꾼 HP가 클라이언트에 도착하면 OnRep_HP에서 UI 알림을 다시 보냅니다.
	UPROPERTY(ReplicatedUsing = OnRep_HP, BlueprintReadOnly, Category = "Health")
	float HP = 100.f;

	// 사망 알림이 여러 번 나가지 않도록 막는 스위치입니다.
	UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "Health")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_HP();

	UFUNCTION()
	void OnRep_IsDead();

private:
	// 서버에서 직접 HP가 바뀐 경우와, 클라이언트가 복제로 받은 경우를 같은 방식으로 UI에 알립니다.
	void BroadcastHPChanged();
};
