

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SonarPingMarkerActor.generated.h"

class UStaticMeshComponent;
class UAudioComponent;

// 소나 핑 마거 - 리플리케이트 하지 않음
// 각 클라이언트가 자기 로컬에 직접 스폰
UCLASS()
class GOHOME_API ASonarPingMarkerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ASonarPingMarkerActor();

	// 스폰 직후 소나가 호출 - 이 시간이 지나면 스스로 사라짐
	void InitMarker(float Lifetime);

protected:

	virtual void BeginPlay() override;
	
	// 메쉬와 머터리얼은 BP 서브클래스에서 지정
	UPROPERTY(VisibleAnywhere, Category = "Marker")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	// 3D 감쇠 핑 사운드. 시야 밖 마커를 방향으로 찾게 해주는 핵심 장치
	UPROPERTY(VisibleAnywhere, Category = "Marker")
	TObjectPtr<UAudioComponent> PingAudio;

	// 핑 소리 반복 주기(초). 머티리얼 파동 주기도 이 값에 맞춰 자동 설정
	UPROPERTY(EditAnywhere, Category = "Marker")
	float PingInterval = 2.f;

private:

	FTimerHandle PingAudioTimerHandle;

	void PlayPingSound();
};
