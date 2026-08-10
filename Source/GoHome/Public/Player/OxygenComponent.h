

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

	// UI 기준 산소 칸 수. 기본 15칸.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Oxygen")
	float MaxOxygen = 15.f;

	UPROPERTY(ReplicatedUsing = OnRep_Oxygen, BlueprintReadOnly, Category = "Oxygen")
	float Oxygen = 15.f;

	// 산소가 가득 찬 상태에서 기본적으로 버티는 시간. 600초 = 10분.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float TargetOxygenDuration = 600.f;

	// 무게 1당 추가 산소 소모량. 실제 무게 연결은 IWeightProvider 단계에서 붙인다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float WeightDrainMultiplier = 0.f;

	// 산소가 0일 때 1초마다 들어가는 질식 데미지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float SuffocationDamagePerSecond = 5.f;

	// UI에서 15칸 산소 표시할 때 사용할 값.
	UFUNCTION(BlueprintPure, Category = "Oxygen")
	int32 GetDisplayedOxygenPips() const;

	// UI 게이지나 디버그 확인용 퍼센트.
	UFUNCTION(BlueprintPure, Category = "Oxygen")
	float GetOxygenPercent() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float CalculateOxygenDrainRate() const;

	UFUNCTION()
	void OnRep_Oxygen();
};
