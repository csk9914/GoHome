//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SubmarineDock.generated.h"

class UBoxComponent;

/**
 * 잠수정 내부 판정 볼륨.
 * 문이 닫히는 순간 이 박스 안에 없는 플레이어를 걸러내는 기준으로 쓴다 (도킹 스레드 판정).
 */
UCLASS()
class GOHOME_API ASubmarineDock : public AActor
{
	GENERATED_BODY()

public:
	ASubmarineDock();

	// 레벨/BP에서 위치·크기를 눈으로 맞추도록 EditAnywhere.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Docking Door")
	TObjectPtr<UBoxComponent> InteriorVolume;
};
