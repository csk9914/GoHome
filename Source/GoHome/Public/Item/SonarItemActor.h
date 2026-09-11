

#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "SonarItemActor.generated.h"

class ASonarPingMarkerActor;
class USoundBase;

UCLASS()
class GOHOME_API ASonarItemActor : public AUsableItemBase
{
	GENERATED_BODY()
	
	
public:

	ASonarItemActor();

	// 좌클릭 진입점, 서버에서만 호출
	virtual void ServerUseSpecialAction() override;

	// 쿨다운 중이면 false, 인벤토리가 호출 전 이 값을 확인함
	virtual bool CanUse() const override;

protected:

	// 탐지 반경
	UPROPERTY(EditAnywhere, Category = "Sonar")
	float BaseDetectRadius = 4000.f;

	// 이 반경 안쪽은 탐지 제외
	UPROPERTY(EditAnywhere, Category = "Sonar")
	float MinDetectRadius = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Sonar")
	float UseCooldown = 3.f;

	UPROPERTY(EditAnywhere, Category = "Sonar")
	float MarkerLifetime = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Sonar")
	TSubclassOf<ASonarPingMarkerActor> MarkerClass;

	UPROPERTY(EditAnywhere, Category = "Sonar")
	float MarkerShareRadius = 5000.f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 탐지 실패 시 재생. 소음은 이미 지불했는데 소득이 없는 상황을 플레이어가 "고장"이 아니라 "없음"으로 읽게 하기 위한 피드백
	UPROPERTY(EditDefaultsOnly, Category = "Sonar")
	TObjectPtr<USoundBase> FailSound;

private:

	// 탐지 조건에 맞는 것 중 가장 가까운 하나를 고름(서버 전용)
	AActor* FindNearestDetectable(const FVector& Origin) const;

	// 마지막 사용 시각 - 초기값이 크게 음수라 첫 사용은 항상 통과
	float LastUseTime = -1000.f;

	// 서버가 판정 결과를 여기 세팅하면 각 클라가 OnRep에서 각자 로컬로 마커를 만듦
	// 액터 참조가 아니라 좌표를 보내는 이유: 먼 거리의 아이템 액터는 relevancy에서 잘려 클라이언트에 존재하지 않을 수 있음
	UPROPERTY(Replicated)
	FVector_NetQuantize PingLocation;

	bool ShouldShowMarkerLocally() const;

	UPROPERTY(Replicated)
	bool bPingFound = false;

	// 값이 바뀌는 게 핑을 쐈다는 신호
	UPROPERTY(ReplicatedUsing = OnRep_PingEventId)
	int32 PingEventId = 0;

	UFUNCTION()
	void OnRep_PingEventId();

	// 로컬 연출. 서버와 각 클라이언트가 각자 실행
	void PlayPingCosmetic();

	// 이 화면의 주인이 소나를 들고 있는(=발사한) 사람인지
	bool IsLocallyHeld() const;
};
