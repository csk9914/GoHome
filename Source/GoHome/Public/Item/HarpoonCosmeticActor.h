

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HarpoonCosmeticActor.generated.h"

class UStaticMeshComponent;

// 순수 연출용 액터(판정 없음, 리플리케이트 안함).
// 시작점/끝점을 받아 왕복 이동만 하고 끝나면 스스로 파괴.
// 각 클라이언트가 로컬로 각자 스폰해서 재생(완전히 동일한 프레임일 필요 없는 코스메틱).

UCLASS()
class GOHOME_API AHarpoonCosmeticActor : public AActor
{
	GENERATED_BODY()
	
public:	

	
	AHarpoonCosmeticActor();

	void Play(const FVector& InStart, const FVector& InEnd, float InOutboundDuration, float InReturnDuration);
	
protected:

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Harpoon")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

private:

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	float OutboundDuration = 0.2f;
	float ReturnDuration = 0.4f;
	float Elapsed = 0.f;
	
};
