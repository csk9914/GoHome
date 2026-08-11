/*

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/CarryWeightProvider.h"
#include "Interaction/WeightProvider.h"
#include "CarryWeightComponent.generated.h"

UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class GOHOME_API UCarryWeightComponent : public UActorComponent, public ICarryWeightProvider
{
	GENERATED_BODY()

public:
	UCarryWeightComponent();

	virtual float GetCurrentCarryWeight() const override;
	virtual float GetMaxCarryWeight() const override;
	virtual float GetOverweightAmount() const override;

protected:
	virtual void BeginPlay() override;

	// 캐릭터의 기본 최대 허용 무게. BP에서 캐릭터마다 조정 가능.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry Weight", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseMaxCarryWeight = 100.f;

	// 업그레이드 시스템이 적용한 최대 허용 무게 보너스.
	// 외부에서는 Set 함수로만 변경한다.
	UPROPERTY(BlueprintReadOnly, Category = "Carry Weight")
	float MaxCarryWeightBonus = 0.f;

	// 버프/디버프 시스템이 적용한 임시 최대 허용 무게 보정값.
	// 외부에서는 Set 함수로만 변경한다.
	UPROPERTY(BlueprintReadOnly, Category = "Carry Weight")
	float TemporaryMaxCarryWeightModifier = 0.f;

private:
	// 같은 Owner에 붙은 IWeightProvider들을 저장한다. 보통 InventoryComponent가 들어간다.
	UPROPERTY()
	TArray<TScriptInterface<IWeightProvider>> CachedWeightProviders;

	void FindWeightProviderComponents();
	float GetTotalWeightFromProviders() const;

public:
	UFUNCTION(BlueprintCallable, Category = "Carry Weight")
	void SetMaxCarryWeightBonus(float NewBonus);

	UFUNCTION(BlueprintCallable, Category = "Carry Weight")
	void SetTemporaryMaxCarryWeightModifier(float NewModifier);
};

*/