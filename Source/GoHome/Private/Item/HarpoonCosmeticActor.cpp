
#include "Item/HarpoonCosmeticActor.h"
#include "Components/StaticMeshComponent.h"



AHarpoonCosmeticActor::AHarpoonCosmeticActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // 순수 로컬 연출 -> 각 클라이언트가 알아서 스폰하므로 리플리케이트 불필요.

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	RootComponent = HeadMesh;
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 연출용 -> 판정은 이미 끝난 상태.
}

void AHarpoonCosmeticActor::Play(const FVector& InStart, const FVector& InEnd, 
	                             float InOutboundDuration, float InReturnDuration)
{
	Start = InStart;
	End = InEnd;
	OutboundDuration = InOutboundDuration;
	ReturnDuration = InReturnDuration;
	Elapsed = 0.f;
	SetActorLocation(Start);
	SetActorRotation((End - Start).Rotation());
}

void AHarpoonCosmeticActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Elapsed += DeltaTime;

	if (Elapsed < OutboundDuration)
	{
		SetActorLocation(FMath::Lerp(Start, End, Elapsed / OutboundDuration));
	}

	else if (Elapsed < OutboundDuration + ReturnDuration)
	{
		const float ReturnAlpha = (Elapsed - OutboundDuration) / ReturnDuration;
		SetActorLocation(FMath::Lerp(End, Start, ReturnAlpha));
	}

	else
	{
		Destroy();
	}

}

