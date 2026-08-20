

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeightPlateActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UDockingDoorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeightPlateProgressChanged, int32, FilledPips, int32, TotalPips);

// 발판 위 합산 무게가 임계값을 넘는 순간 문을 연다.
// OpenDuration 초 동안은 무게가 바뀌어도 무시하고 열린 채 유지하다 자동으로 닫힌다.
// 미끼 아이템으로 열고 안에서 버린 뒤 진짜 값어치 있는 물건을 챙겨 나오는 플레이 허용.


UCLASS()
class GOHOME_API AWeightPlateActor : public AActor
{
	GENERATED_BODY()
	
public:	

	AWeightPlateActor();

	// 시각 피드백(원이 몇개 찼는지 등)용. UI/이펙트가 참조.
	UFUNCTION(BlueprintPure, Category = "Weight Plate")
	int32 GetFilledPipCount() const;

	UFUNCTION(BlueprintPure, Category = "Weight Plate")
	int32 GetTotalPipCount() const;

	// 채워진 정도가 바뀔 때마다 브로드 캐스트(폴링 없이 갱신 가능하게).
	UPROPERTY(BlueprintAssignable, Category = "Weight Plate")
	FOnWeightPlateProgressChanged OnProgressChanged;

protected:
	
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "Weight Plate")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Weight Plate")
	TObjectPtr<UBoxComponent> TriggerVolume;

	// 이 발판이 여닫을 문이 붙은 액터. 컴포넌트는 에디터에서 직접 못 골라서(에셋 피커로만 뜸) 액터로 지정.
	UPROPERTY(EditAnywhere, Category = "Weight Plate")
	TObjectPtr<AActor> TargetDoorActor;

	// 문이 열리는 데 필요한 합산 무게.
	UPROPERTY(EditAnywhere, Category = "Weight Plate", meta = (ClampMin = "0.0"))
	float WeightThreshold = 8.f;

	// 문이 열린 채 유지되는 시간(초). 이 동안은 무게가 바뀌어도 무시.
	UPROPERTY(EditAnywhere, Category = "Weight Plate", meta = (ClampMin = "0.1"))
	float OpenDuration = 15.f;

	// 무게 재확인 주기(초).
	UPROPERTY(EditAnywhere, Category = "Weight Plate", meta = (ClampMin = "0.05"))
	float CheckInterval = 0.2f;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:

	void EvaluatePlateState();
	void CloseAfterDuration();
	float CalculateCurrentWeight() const;
	
	UFUNCTION()
	void OnRep_CurrentWeight();

	UPROPERTY()
	TSet<TObjectPtr<APawn>> OverlappingPawns;

	// TargetDoorActor에서 BeginPlay 때 자동으로 찾아 캐시.
	UPROPERTY()
	TObjectPtr<UDockingDoorComponent> TargetDoor;

	// 시각 피드백용 현재 합산 무게(클라에도 보여야 하므로 리플리케이트).
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeight)
	float CachedCurrentWeight = 0.f;

	// 이미 이 발판이 문을 열어서 OpenDuration 카운트다운 중인지(재트리거 방지).
	bool bIsTriggered = false;

	FTimerHandle CheckTimerHandle;
	FTimerHandle CloseTimerHandle;

};
