

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportStationActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;


// 복귀 잠수정 내부에 배치되는 텔레포트 기계
UCLASS()
class GOHOME_API ATeleportStationActor : public AActor
{
	GENERATED_BODY()

public:
	ATeleportStationActor();

	// 착지 위치/회전. 리모컨이 텔레포트 직전에 읽음
	UFUNCTION(BlueprintPure, Category = "Teleport")
	FTransform GetTeleportTransform() const;

	// 서버 전용. 리모컨이 채널 시작/취소 시 호출
	void SetCharging(bool bNewCharging);

	UFUNCTION(BlueprintPure, Category = "Teleport")
	bool IsCharging() const { return bIsCharging; }

	// 도착 순간 1회 연출. 서버에서 호출 -> 전원 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayArrivalBurst();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Teleport")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Teleport")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 착지 지점. 에디터에서 기계 앞쪽으로 옮겨 둠
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<USceneComponent> TeleportTarget;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 충전 상태가 바뀔 때 각 클라에서 호출. 기계 지직/발광 연출을 BP에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "Teleport")
	void OnChargingChanged(bool bCharging);

	// 도착 순간 1회 플래시. BP에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "Teleport")
	void OnArrivalBurst();

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsCharging)
	bool bIsCharging = false;

	UFUNCTION()
	void OnRep_IsCharging();
};