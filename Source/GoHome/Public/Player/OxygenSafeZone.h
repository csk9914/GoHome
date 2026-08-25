#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OxygenSafeZone.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
struct FHitResult;

UCLASS()
class GOHOME_API AOxygenSafeZone : public AActor
{
	GENERATED_BODY()

public:
	AOxygenSafeZone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 안전지대 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Oxygen Safe Zone")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Oxygen Safe Zone")
	TObjectPtr<UBoxComponent> ZoneVolume;

	// [확인 대기 시간] 게임 시작 후 플레이어가 이미 안에 서 있는지 확인할 때까지 기다릴 시간 (0.2초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen Safe Zone", meta = (ClampMin = "0.0"))
	float InitialOverlapCheckDelay = 0.2f;

private:
	// [타이머 번호표] 0.2초 딜레이 타이머를 켜고 끌 때 사용하는 식별용 번호표
	FTimerHandle InitialOverlapCheckTimerHandle;

	// [입장 감지 센서] 누군가 박스 안으로 "발을 들여놓았을 때" 자동으로 켜지는 함수
	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// [퇴장 감지 센서] 누군가 박스 밖으로 "완전히 나갔을 때" 자동으로 켜지는 함수
	UFUNCTION()
	void OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// [주변 다시 둘러보기] 시작하자마자 박스 안에 이미 들어와 있는 캐릭터가 있는지 싹 훑어보는 함수
	void RefreshOverlappingActors();

	// [산소 스위치 켜고 끄기] 들어온 대상(Actor)에게 "너 지금 안전지대야!(true)" 또는 "밖이야!(false)"라고 알려주는 함수
	void SetActorInSafeZone(AActor* Actor, bool bNewInSafeZone) const;
};