#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BreathBubbleComponent.generated.h"

class UNiagaraSystem;
class USkeletalMeshComponent;

/**
 * 소유 액터의 지정한 소켓에서, 일정하지 않은 간격으로 호흡 이펙트(공기방울)를 재생한다.
 *
 * 순수 연출용이라 리플리케이트하지 않는다.
 * 액터의 위치와 속도는 엔진이 이미 동기화하므로, 각 머신이 로컬에서 판단해 재생하면
 * 모두에게 자연스럽게 보인다. 덕분에 RPC가 하나도 필요 없다.
 */
UCLASS(ClassGroup = (FX), meta = (BlueprintSpawnableComponent))
class GOHOME_API UBreathBubbleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBreathBubbleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 지금 즉시 한 번 뿜는다. 피격, 놀람, 컷신 등에서 직접 호출해도 된다.
	UFUNCTION(BlueprintCallable, Category = "Breath")
	void TriggerBreath();

protected:
	virtual void BeginPlay() override;

	// 호흡 한 번에 재생할 나이아가라 시스템
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breath")
	TObjectPtr<UNiagaraSystem> BreathBubbleFX;

	// 방울이 나올 소켓 이름. 스켈레톤에 만들어 둔 이름과 정확히 일치해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breath")
	FName BreathSocketName = "BubbleSocket";

	// 멈춰 있을 때의 호흡 간격 범위(초). 매번 이 사이에서 랜덤으로 뽑는다.
	// 두 값의 차이가 클수록 불규칙해져서 자연스럽다.
	UPROPERTY(EditAnywhere, Category = "Breath|Interval", meta = (ClampMin = "0.1"))
	float BreathIntervalMin = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Breath|Interval", meta = (ClampMin = "0.1"))
	float BreathIntervalMax = 6.0f;

	// 이 속도(cm/s)를 넘으면 "움직이는 중"으로 본다.
	UPROPERTY(EditAnywhere, Category = "Breath|Interval", meta = (ClampMin = "0.0"))
	float MoveSpeedThreshold = 50.f;

	// 움직이는 중일 때 호흡 간격에 곱할 값. 1보다 작을수록 숨이 가빠진다.
	// 1.0으로 두면 속도와 무관하게 항상 같은 간격이 된다.
	UPROPERTY(EditAnywhere, Category = "Breath|Interval", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float MovingIntervalScale = 0.6f;

	// 호흡 간격의 하한선. 이보다 촘촘해지지 않는다.
	UPROPERTY(EditAnywhere, Category = "Breath|Interval", meta = (ClampMin = "0.05"))
	float MinBreathInterval = 0.25f;

private:
	// 현재 상태(정지 / 이동)에 맞는 다음 호흡 간격을 뽑는다.
	float PickNextInterval() const;

	// 이펙트를 붙일 스켈레탈 메시를 찾는다.
	USkeletalMeshComponent* ResolveMesh() const;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	// 마지막 호흡 이후 누적 시간
	float BreathTimer = 0.f;

	// 이번에 뽑힌, 다음 호흡까지의 목표 시간
	float NextBreathInterval = 0.f;
};
