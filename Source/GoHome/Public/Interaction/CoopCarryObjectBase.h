

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

	UPROPERTY(VisibleAnywhere, Category = "CoopCarry")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 오브젝트 외곽 양쪽 - 각 캐리어가 서는/붙잡는 위치(에디터에서 배치).
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

	// 빈 핸들에 폰을 배정. 성공하면 true;
	bool AssignCarrier(APawn* Pawn);

	// CarryData의 메쉬/스케일을 실제 컴포넌트에 반영.
	void SyncFromCarryData();

	// 정체(손발 안 맞음) 판정 - 둘 다 입력 중인데 합산 결과가 이 값보다 작은 상태가
	// 이 시간(초) 이상 지속되면 강제로 놓침.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float StuckInputThreshold = 0.1f;

	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float StuckDropDuration = 2.0f;

	// 합산 이동 벡터에 곱해지는 배율.
	// 무거운 물건이라 느리게 하고 싶으면 1보다 작게 세팅.
	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float CarrySpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "CoopCarry")
	float MaxCarryDistance = 500.f;

	float TimeStuck = 0.f;

};
