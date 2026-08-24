#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/CarryWeightProvider.h"
#include "Interaction/WeightProvider.h"
#include "CarryWeightComponent.generated.h"

/**
 * 플레이어의 운반 무게 상태를 계산한다.
 *
 * IWeightProvider(예: InventoryComponent)에서 현재 아이템 무게를 읽고,
 * 캐릭터의 최대 허용 무게와 비교해 초과 무게를 제공한다.
 */
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class GOHOME_API UCarryWeightComponent : public UActorComponent, public ICarryWeightProvider
{
	GENERATED_BODY()

public:
	UCarryWeightComponent();

	virtual float GetCurrentCarryWeight() const override;
	
	UFUNCTION(BlueprintPure, Category = "Carry Weight")
	virtual float GetMaxCarryWeight() const override;

	UFUNCTION(BlueprintPure, Category = "Carry Weight")
	virtual float GetOverweightAmount() const override;

protected:
	virtual void BeginPlay() override;

	// InventoryComponent를 직접 호출하지 않고, 짧은 주기로 IWeightProvider 값을 다시 확인한다.
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	// 캐릭터가 기본으로 버틸 수 있는 최대 무게.
	// 업그레이드가 없는 상태의 기준값이며, 캐릭터/BP별로 조정할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry Weight", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseMaxCarryWeight = 7.f;

	// 업그레이드 시스템이 더해주는 최대 무게 보너스.
	// 외부 시스템은 값에 직접 접근하지 않고 Set 함수로만 변경한다.
	UPROPERTY(BlueprintReadOnly, Category = "Carry Weight")
	float MaxCarryWeightBonus = 0.f;

	// 버프/디버프처럼 일시적으로 적용되는 최대 무게 보정값.
	// 디버프 표현을 위해 음수도 허용한다.
	UPROPERTY(BlueprintReadOnly, Category = "Carry Weight")
	float TemporaryMaxCarryWeightModifier = 0.f;

	// 마지막으로 계산된 현재 운반 무게.
	// GetCurrentCarryWeight()는 매번 합산하지 않고 이 저장값을 반환한다.
	UPROPERTY(BlueprintReadOnly, Category = "Carry Weight")
	float CurrentCarryWeight = 0.f;


private:
	// 같은 Owner에 붙은 무게 제공자들을 캐시한다.
	// 보통 InventoryComponent가 들어가며, 구체 클래스 대신 IWeightProvider만 의존한다.
	UPROPERTY()
	TArray<TScriptInterface<IWeightProvider>> CachedWeightProviders;

	// BeginPlay 이후 Owner의 컴포넌트 중 IWeightProvider 구현체를 찾아 캐시한다.
	void FindWeightProviderComponents();

	// 캐시된 IWeightProvider들의 무게를 모두 더한다.
	float GetTotalWeightFromProviders() const;

	// 0.1초마다 현재 운반 무게를 갱신한다.
	void RefreshCurrentCarryWeight();

public:
	UFUNCTION(BlueprintCallable, Category = "Carry Weight")
	void SetMaxCarryWeightBonus(float NewBonus);

	UFUNCTION(BlueprintCallable, Category = "Carry Weight")
	void SetTemporaryMaxCarryWeightModifier(float NewModifier);
};
