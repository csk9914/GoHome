#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "HammerItemActor.generated.h"

class ABreakableWallActor;

// 부술 수 있는 벽(ABreakableWallActor)에 대고 좌클릭 -> 즉시 한 대 타격.
// 지속 상태 없음(쿨다운만) - 배터리 없이 무제한 사용 가능.
UCLASS()
class GOHOME_API AHammerItemActor : public AUsableItemBase
{
	GENERATED_BODY()

public:
	virtual void ServerUseSpecialAction() override;
	virtual bool CanUse() const override;
	virtual bool IsDeliverable() const override { return false; }

protected:
	UPROPERTY(EditAnywhere, Category = "Hammer")
	float TraceDistance = 200.f;

	UPROPERTY(EditAnywhere, Category = "Hammer")
	float TraceRadius = 15.f;

	UPROPERTY(EditAnywhere, Category = "Hammer")
	float UseCooldown = 1.f;

	// 타격 지점 소켓(해머 머리 끝). 없으면 근사치.
	UPROPERTY(EditAnywhere, Category = "Hammer")
	FName ImpactSocketName = NAME_None;

	// 휘두르기 시작 -> 실제 타격 판정까지 지연(초).
	// AM_Hammer_Swing에서 망치가 가장 아래로 내려오는 순간에 맞춘다. 0이면 즉시 판정(기존 동작).
	UPROPERTY(EditAnywhere, Category = "Hammer", meta = (ClampMin = "0.0"))
	float ImpactDelay = 0.5f;

private:
	FVector GetImpactLocation() const;
	FVector GetAimDirection() const;

	// 실제 스윕 판정. ImpactDelay 후 타이머로 호출된다(서버 전용).
	void PerformImpactTrace();

	float LastUseTime = -1000.f;

	// 타격 판정 지연 타이머.
	FTimerHandle ImpactTimerHandle;

	// 휘두르기 시작 시점의 사용자. 지연 도중 다른 사람이 주워 가면 판정을 취소하기 위함.
	TWeakObjectPtr<APawn> SwingPawn;
};