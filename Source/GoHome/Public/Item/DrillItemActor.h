

#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "AI/NoiseType.h"
#include "DrillItemActor.generated.h"

class ABreakableWallActor;
class UNiagaraComponent;
class UAudioComponent;

// 부술 수 있는 벽에 대고 좌클릭 -> 몇 초간 드릴 -> 완료 시 벽 파괴.
// 드릴링 중 계속 소음, 시선/거리 벗어나면 중단.
// 배터리 소진 시 AUsableItemBase 인프라로 자동 소멸.

UCLASS()
class GOHOME_API ADrillItemActor : public AUsableItemBase
{
	GENERATED_BODY()
	

public:

	ADrillItemActor();

	virtual void ServerUseSpecialAction() override;
	virtual bool CanUse() const override;
	virtual bool IsDeliverable() const override { return false; }

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ServerDrop() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "Drill|FX")
	TObjectPtr<UNiagaraComponent> DrillVFX;

	UPROPERTY(VisibleAnywhere, Category = "Drill|FX")
	TObjectPtr<UAudioComponent> DrillSFX;

	UPROPERTY(EditAnywhere, Category = "Drill")
	float TraceDistance = 300.f;

	UPROPERTY(EditAnywhere, Category = "Drill")
	float TraceRadius = 15.f;

	UPROPERTY(EditAnywhere, Category = "Drill")
	float DrillDuration = 3.f;

	UPROPERTY(EditAnywhere, Category = "Drill")
	float UseCooldown = 1.f;

	UPROPERTY(EditAnywhere, Category = "Drill")
	int32 BatteryCharges = 3;

	// 드릴 팁 소켓, 없으면 근사치.
	UPROPERTY(EditAnywhere, Category = "Drill")
	FName MuzzleSocketName = NAME_None;

	// 드릴링 중 지속 소음.
	UPROPERTY(EditAnywhere, Category = "Drill")
	float DrillNoiseRadius = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Drill")
	ENoiseType DrillNoiseType = ENoiseType::Medium;

	UPROPERTY(EditAnywhere, Category = "Drill")
	float DrillNoiseInterval = 1.f;

private:

	FVector GetMuzzleLocation() const;
	FVector GetAimDirection() const;
	ABreakableWallActor* TraceForWall() const;
	void AbortDrill();
	void SetDrillCosmeticActive(bool bActive);

	UFUNCTION()
	void OnRep_IsDrilling();

	UPROPERTY()
	TObjectPtr<ABreakableWallActor> DrillingTarget;

	UPROPERTY(ReplicatedUsing = OnRep_IsDrilling)
	bool bIsDrilling = false;

	UPROPERTY(Replicated)
	int32 ChargesRemaining = 0;

	float DrillElapsed = 0.f;
	float DrillNoiseTimer = 0.f;
	float LastUseTime = -1000.f;
};
