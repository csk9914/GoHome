#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "SharedLockerActor.generated.h"

class APawn;
class UStaticMeshComponent;

/**
 * 잠수정 공유 보관함 상자. 상호작용하면 그 플레이어에게 보관함 UI를 연다 — 상태는 갖지 않는다.
 * 보관 수량(서버 SoT)은 UItemShopSubsystem + SaveGame, 클라 표시는 AGoHomeGameState 복제 미러.
 * 서버는 꺼내기/넣기 RPC마다 요청자가 UseRadius 안에 있는지 다시 검사한다.
 */
UCLASS()
class GOHOME_API ASharedLockerActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASharedLockerActor();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

	// 서버 RPC 검증용 — UI가 열린 채로 멀어지면 꺼내기/넣기가 거절된다.
	bool IsPawnInUseRange(const APawn* Pawn) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shared Locker")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 상호작용 트레이스 거리보다 넉넉하게 — UI를 연 위치에서 조금 움직여도 쓸 수 있게.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shared Locker", meta = (ClampMin = "50.0"))
	float UseRadius = 400.f;
};
