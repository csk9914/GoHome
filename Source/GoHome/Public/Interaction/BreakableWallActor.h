

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/NoiseType.h"
#include "BreakableWallActor.generated.h"

class UStaticMeshComponent;
class ANavLinkProxy;
class AGeometryCollectionActor;
class UNiagaraSystem;
class USoundBase;

// 드릴류 아이템으로 부술 수 있는 벽.
// 상호작용(E) 대상이 아니라 아이템 트레이스가 Cast로 찾는다.
// 권위 : bIsBroken 하나가 "통행 가능 여부"의 유일한 진실.
// 파편(Geometry Collection)은 순수 로컬 코스메틱.

UCLASS()
class GOHOME_API ABreakableWallActor : public AActor
{
	GENERATED_BODY()
	
public:	

	ABreakableWallActor();

	// 드릴 아이템이 발사 전/드릴링 중 확인.
	bool IsBroken() const { return bIsBroken; }
	float GetDrillTimeMultiplier() const { return DrillTimeMultiplier; }

	// 서버 권위 : 드릴 완료 시 호출.
	void ServerBreak(AActor* InInstigator, const FVector& HitDirection);

protected:

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category = "BreakableWall")
	TObjectPtr<UStaticMeshComponent> IntactMesh;

	UPROPERTY(VisibleAnywhere, Category = "BreakableWall")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	// 부서질 때 통로 통행을 여는 내비 링크. 레벨 디자이너가 통로에 배치 후 여기 연결(기본 비활성).
	UPROPERTY(EditInstanceOnly, Category = "BreakableWall")
	TObjectPtr<ANavLinkProxy> PassageNavLink;

	// 드릴 시간 배율(벽별 난이도). 드릴 아이템이 DrillDuration에 곱함.
	UPROPERTY(EditAnywhere, Category = "BreakableWall")
	float DrillTimeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "BreakableWall")
	float BreakNoiseRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category = "BreakableWall")
	ENoiseType BreakNoiseType = ENoiseType::Large;

	// --- 코스메틱 ---
	UPROPERTY(EditAnywhere, Category = "BreakableWall|FX")
	TSubclassOf<AGeometryCollectionActor> ShatterActorClass;

	UPROPERTY(EditAnywhere, Category = "BreakableWall|FX")
	float ShatterImpulseStrength = 400.f;

	UPROPERTY(EditAnywhere, Category = "BreakableWall|FX")
	float ShatterLifespan = 5.f;

	UPROPERTY(EditAnywhere, Category = "BreakableWall|FX")
	TObjectPtr<UNiagaraSystem> BreakEffect;

	UPROPERTY(EditAnywhere, Category = "BreakableWall|FX")
	TObjectPtr<USoundBase> BreakSound;

public:	

	UPROPERTY(ReplicatedUsing = OnRep_IsBroken)
	bool bIsBroken = false;

	UFUNCTION()
	void OnRep_IsBroken();

	// bPlayEffects = false면 메시/콜리전 스왑만(늦게 접속한 클라의 상태 동기화용).
	void ApplyBrokenState(bool bPlayEffects);

};
