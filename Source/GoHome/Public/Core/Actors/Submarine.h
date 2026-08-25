//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Submarine.generated.h"

class UBoxComponent;

/**
 * 로비/탐사 양쪽 맵에 배치되는 잠수정 오브젝트.
 * 이 클래스 범위: 외형(허브/문 메시), 내부 판정 볼륨, 문 닫힘 반응(세이프존 밖 플레이어 처리).
 * 이 클래스 범위 밖: 이동/조종 로직 없음 — 잠수정은 정적 배치물이며 UDockingDoorComponent(GameState 소유)의
 * 공개 상태만 구독한다 (GameState 전체 참조 금지).
 */
UCLASS()
class GOHOME_API ASubmarine : public AActor
{
	GENERATED_BODY()

public:
	ASubmarine();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDoorStateChanged(bool bOpen); 
	
public:
	// 레벨/BP에서 위치·크기를 눈으로 맞추도록 EditAnywhere.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Docking Door")
	TObjectPtr<UBoxComponent> InteriorVolume;
};
