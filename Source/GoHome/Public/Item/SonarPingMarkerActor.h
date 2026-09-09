

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SonarPingMarkerActor.generated.h"

class UStaticMeshComponent;

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
	
	// 메쉬와 머터리얼은 BP 서브클래스에서 지정
	UPROPERTY(VisibleAnywhere, Category = "Marker")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

};
