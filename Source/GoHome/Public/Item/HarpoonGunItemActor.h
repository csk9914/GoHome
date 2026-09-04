

#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "HarpoonGunItemActor.generated.h"


class AItemActorBase;
class AHarpoonCosmeticActor;

// 오른손 사용 아이템 : 좌클릭으로 원거리(좁은 틈 / 멀리있는) 일반 아이템을 끌어옴.
// 2인 협동 오브젝트(CoopCarryObject)는 타입 필터로 자동 제외.
// 판정은 발사 즉시 트레이스로 확정(빗나가지 않음) -> 왕복 연출은 순수 코스메틱(AHarpoonCosmeticActor).
// 회수된 아이템은 자동으로 인벤토리에 안 들어가고 플레이어 근처에 내려놓기만 함 -> 상호작용(E)하여 주워야 함.


UCLASS()
class GOHOME_API AHarpoonGunItemActor : public AUsableItemBase
{
	GENERATED_BODY()
	
public:
	
	virtual void ServerUseSpecialAction() override;
	virtual bool CanUse() const override;
	virtual bool IsDeliverable() const override { return false; }

protected:

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Category = "Harpoon")
	float TraceDistance = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Harpoon")
	float TraceRadius = 20.f;

	UPROPERTY(EditAnywhere, Category = "Harpoon")
	float UseCooldown = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Harpoon")
	float OutboundDuration = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Harpoon")
	float ReturnDuration = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Harpoon")
	float DropOffDistance = 150.f;


private:

	UPROPERTY()
	TObjectPtr<AItemActorBase> RetrievingTarget;

	FVector ReturnStartLocation = FVector::ZeroVector;
	bool bReturnStartCaptured = false;
	float RetrieveElapsed = 0.f;
	float LastUseTime = -1000.f;

	// 코스메틱 트리거용 - 서버가 발사 시점의 시작/끝 두 점을 세팅하면
	// 각 클라이언트가 OnRep_FireEventId에서 각자 로컬로 재생(정확한 프레임 동기화 불필요).
	UPROPERTY(Replicated)
	FVector_NetQuantize FireEventStart;

	UPROPERTY(Replicated)
	FVector_NetQuantize FireEventEnd;

	UPROPERTY(ReplicatedUsing = OnRep_FireEventId)
	int32 FireEventId = 0;

	UFUNCTION()
	void OnRep_FireEventId();

	void PlayFireCosmetic();
	void AbortRetrieve();

	// 발사 연출로 스폰할 코스메틱 액터 클래스. 메쉬를 세팅한 블루프린트 서브클래스를 지정할 것.
	UPROPERTY(EditDefaultsOnly, Category = "Harpoon")
	TSubclassOf<AHarpoonCosmeticActor> CosmeticClass;

};
