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

private:
	FVector GetImpactLocation() const;
	FVector GetAimDirection() const;

	float LastUseTime = -1000.f;
};