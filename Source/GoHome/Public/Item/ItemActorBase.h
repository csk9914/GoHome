

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Interaction/WeightProvider.h"
#include "ItemActorBase.generated.h"

class UItemDataAsset;
class UStaticMeshComponent;

/**
 * 파손형 충돌 감지, 소음 유발형 누적 타이머는 아이템 자신이 갖는다
 * (인벤토리는 무게/개수만 알면 되고 아이템별 상태 로직을 몰라도 되게 하기 위함).
 */
UCLASS()
class GOHOME_API AItemActorBase : public AActor, public IInteractable, public IWeightProvider
{
	GENERATED_BODY()

public:
	AItemActorBase();

	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemDataAsset> ItemData;

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	virtual float GetTotalWeight() const override;

	// 파손 반영된 현재 가치. 납품 정산 시 ItemData->Value대신 이 값을 사용해야 함.
	UFUNCTION(BlueprintCallable, Category = "Item")
	float GetCurrentValue() const;

	// 인벤토리에 담길 때 Interaction이 호출한다(소음 유발형이면 반경 증가 타이머 시작).
	void NotifyPickedUp();

	// 인벤토리에서 빠질 때 Interaction이 호출한다 (타이머 정지 + 반경 리셋).
	void NotifyDropped();

	// 드롭 시 앞으로 던져지는 초기 속도. 값 낮추면 툭 떨어지고, 높이면 멀리 날아간다.
	UPROPERTY(EditAnywhere, Category = "Item")
	float DropThrowSpeed = 500.f;

	// 서버 권위 : 부착 해제 + 물리/콜리전 복원 + 인벤토리에서 제거.
	// InventoryComponent::Server_RequestDrop이 소유권 검증 후 호출한다. 서버에서만 호출할 것.
	void ServerDrop();

protected:

	virtual void BeginPlay() override;


	UPROPERTY(ReplicatedUsing = OnRep_HoldingPawn)
	TObjectPtr<APawn> HoldingPawn = nullptr;

	UFUNCTION()
	void OnRep_HoldingPawn();

	void UpdateAttachment();




	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void NotifyHit(
		UPrimitiveComponent* MyComp, 
		AActor* Other, 
		UPrimitiveComponent* OtherComp, 
		bool bSelfMoved, 
		FVector HitLocation, 
		FVector HitNormal, 
		FVector NormalImpulse, 
		const FHitResult& Hit) override;

	// 동시 픽업 레이스 컨디션 방지: 서버 틱 단일 스레드 특성 이용 (동기 함수 호출 안에서 끊김 없이 검사+설정).
	UPROPERTY(Replicated)
	bool bIsBeingClaimed = false;

	// 파손형 누적 파손 횟수. ItemData->MaxBreakCount에서 멈춘다.
	UPROPERTY(Replicated)
	int32 BreakCount = 0;

	// 소음 유발형 현재 반경(미보유 시 0).
	UPROPERTY(Replicated)
	float CurrentNoiseRadius = 0.f;

private:

	void GrowNoiseRadius();

	FTimerHandle NoiseGrowthTimerHandle;

};
