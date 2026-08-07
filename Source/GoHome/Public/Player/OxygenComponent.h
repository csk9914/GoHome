

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OxygenComponent.generated.h"

/**
 * 소모 속도 계산 시 IWeightProvider를 참조한다 (UInventoryComponent를 직접 참조하지 않음).
 * 산소 0 도달 시 Owner의 IDamageable::ApplyDamage를 동기 호출한다 (별도 이벤트 계층 없음).
 */
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class GOHOME_API UOxygenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOxygenComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Oxygen")
	float MaxOxygen = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Oxygen, BlueprintReadOnly, Category = "Oxygen")
	float Oxygen = 100.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Oxygen();
};
