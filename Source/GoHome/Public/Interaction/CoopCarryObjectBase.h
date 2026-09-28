

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "CoopCarryObjectBase.generated.h"

class UStaticMeshComponent;
class UCoopCarryDataAsset;

// 2인 협동 운반 오브젝트 베이스.
// 오브젝트 외곽 양쪽 손잡이(HandleA / HandleB)에 각각 한 명씩 붙잡아야 운반이 시작된다.
// 확장할 때는 이 클래스만 상속해서 메쉬/손잡이 위치만 다르게 배치하면 됨(공동 로직은 여기에 다 있음).

UCLASS()
class GOHOME_API ACoopCarryObjectBase : public AActor, public IInteractable
{
	GENERATED_BODY()


public:

	ACoopCarryObjectBase();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

	// 두 캐리어가 다 배정이 되었는지 확인(실제 운반이 시작되는 조건).
	UFUNCTION(BlueprintPure, Category = "CoopCarry")
	bool IsFullyCarried() const { return CarrierA && CarrierB; }

	// 서버 권위: 자발적(Q)이든 강제(피격/사망 등)든 이 함수 하나로 들어옴.
	// 한쪽만 반쪽 상태로 남기지 않고 둘다 같이 해제함.
	void ReleaseCarriers();

	// 서버 권위: 납품 지점에서 호출됨. 정산 후 자기 자신을 파괴함.
	// bIsBeingDelivered로 중복 호출(두 캐리어가 동시에 눌러도) 방지.
	void ServerDeliver();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "CoopCarry")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 오브젝트 외곽 양쪽 - 각 캐리어가 서는 위치(에디터에서 배치, 위치만 사용하고 회전은 무시).
	// 캐리어는 서로 상대 손잡이 쪽을 바라봄. 오브젝트에 붙어 있어서 오브젝트가 움직이거나 돌면 같이 따라감.
	UPROPERTY(VisibleAnywhere, Category = "CoopCarry")
	TObjectPtr<USceneComponent> HandleA;

	UPROPERTY(VisibleAnywhere, Category = "CoopCarry")
	TObjectPtr<USceneComponent> HandleB;
	
	// 이 오브젝트의 정산 가치/메쉬 등 정보.
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_CarryData, Category = "CoopCarry")
	TObjectPtr<UCoopCarryDataAsset> CarryData;

	UFUNCTION()
	void OnRep_CarryData();

	UPROPERTY(ReplicatedUsing = OnRep_Carriers)
	TObjectPtr<APawn> CarrierA;

	UPROPERTY(ReplicatedUsing = OnRep_Carriers)
	TObjectPtr<APawn> CarrierB;

	UFUNCTION()
	void OnRep_Carriers();

	// 정산 중복 방지용(픽업 때 쓰는 bIsBeingClaimed와 동일한 이유 - 서버 틱 단일 스레드 특성 이용).
	UPROPERTY(Replicated)
	bool bIsBeingDelivered = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

private:
private:

	// 빈 핸들에 폰을 배정. 성공하면 true;
	bool AssignCarrier(APawn* Pawn);

	// CarryData의 메쉬/스케일을 실제 컴포넌트에 반영.
	void SyncFromCarryData();

	// 이 위치/회전에 오브젝트를 놓으면 막히는 것과 겹치는지(자기 자신, 캐리어 제외).
	bool IsBlockedAt(const FVector& Location, const FQuat& Rotation) const;

	// 공유 이동 벡터에 곱해지는 배율.
	// 무거운 물건이라 느리게 하고 싶으면 1보다 작게 세팅.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float CarrySpeedScale = 1.0f;

	// 잡는 순간 1회 랜덤으로 정해지는 역할(그 세션 동안 고정). true면 CarrierA가 이동 역할.
	bool bCarrierAIsMover = true;

	// true면 CarrierA -> HandleA, CarrierB -> HandleB. 운반 시작 시 이동 거리 합이 짧은 쪽으로 배정(엇갈림 방지).
	bool bCarrierAOnHandleA = true;

	// 운반 시작 직후 두 캐리어가 손잡이에 도착했는지. 도착 전엔 오브젝트를 고정하고 캐리어만 손잡이로 이동시킴.
	bool bHandlesReached = false;
	float HandleReachElapsed = 0.f;

	// 진행 방향. 운반 시작 시 이동 역할의 시선 Yaw로 초기화되고, 회전 역할의 조향으로 바뀜.
	float HeadingYaw = 0.f;
	float HeadingPitch = 0.f;

	// 회전 역할 조향 속도(초당 각도).
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float SteerYawSpeed = 60.f;

	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float SteerPitchSpeed = 45.f;

	// 진행 방향 최대 기울기(도). 90에 가까우면 한 사람이 다른 사람 바로 위에 서게 되어 좌우 조향이 불안정해짐.
	UPROPERTY(EditAnywhere, Category = "CoopCarry", meta = (ClampMin = "0.0", ClampMax = "80.0"))
	float MaxHeadingPitch = 60.f;

	// 두 캐리어가 손잡이에서 이 거리 이내로 들어오면 "도착"으로 보고 조작 시작.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float HandleArriveTolerance = 20.f;

	// 이 시간 안에 도착 못 하면(장애물 등) 그냥 조작 시작.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float HandleReachTimeout = 1.5f;

	// 캐릭터가 손잡이에서 이 거리만큼 벗어나면 보정 입력이 최대(1)가 됨.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float HandleFollowRange = 100.f;

	// 손잡이와의 오차가 Tolerance를 넘으면 조향이 느려지기 시작하고, StopDistance에서 완전히 멈춤.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float HandleLagTolerance = 30.f;

	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float HandleLagStopDistance = 120.f;

	// 회전 역할이 Shift를 누르고 있을 때 공유 이동 벡터에 곱해지는 배율.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float CarryBoostMultiplier = 1.3f;

	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float MaxCarryDistance = 500.f;
};
