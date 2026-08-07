

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Interaction/WeightProvider.h"
#include "ItemActorBase.generated.h"

class UItemDataAsset;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemDataAsset> ItemData;

	virtual bool CanInteract(APlayerController* PlayerController) const override;
	virtual void OnInteract(APlayerController* PlayerController) override;
	virtual float GetTotalWeight() const override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 동시 픽업 레이스 컨디션 방지: 서버 틱 단일 스레드 특성 이용 (동기 함수 호출 안에서 끊김 없이 검사+설정).
	UPROPERTY(Replicated)
	bool bIsBeingClaimed = false;
};
