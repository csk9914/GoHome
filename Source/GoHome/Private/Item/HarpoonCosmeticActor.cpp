
#include "Item/HarpoonCosmeticActor.h"
#include "CableComponent.h"
#include "Components/StaticMeshComponent.h"



AHarpoonCosmeticActor::AHarpoonCosmeticActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // 순수 로컬 연출 -> 각 클라이언트가 알아서 스폰하므로 리플리케이트 불필요.

	TrajectoryRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TrajectoryRoot"));
	RootComponent = TrajectoryRoot;

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(TrajectoryRoot);
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 연출용 -> 판정은 이미 끝난 상태.

	Cable = CreateDefaultSubobject<UCableComponent>(TEXT("Cable"));
	Cable->SetupAttachment(HeadMesh);
	Cable->SetUsingAbsoluteRotation(true); // 부모(HeadMesh/TrajectoryRoot) 회전 영향을 안 받고 항상 월드 축 기준으로 고정.
	Cable->CableWidth = 1.5f;
	Cable->NumSegments = 1;
	Cable->bEnableStiffness = true; // 축 처지지 않고 팽팽한 느낌
	Cable->SolverIterations = 4;
	Cable->CableGravityScale = 0.f;

}

void AHarpoonCosmeticActor::Play(const FVector& InStart, const FVector& InEnd,
	                             float InOutboundDuration, float InReturnDuration,
	                             UStaticMeshComponent* InMuzzleMesh, FName InMuzzleSocket)
{
	Start = InStart;
	End = InEnd;
	OutboundDuration = InOutboundDuration;
	ReturnDuration = InReturnDuration;
	Elapsed = 0.f;
	MuzzleMesh = InMuzzleMesh;
	MuzzleSocket = InMuzzleSocket;
	SetActorLocation(Start);
	SetActorRotation((End - Start).Rotation());
}

void AHarpoonCosmeticActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Elapsed += DeltaTime;

	// 총구의 "현재" 위치를 매 틱 다시 읽음 - 발사 후 플레이어가 시점을 바꿔도 따라가야 하므로.
	FVector CurrentMuzzleLocation = Start;
	if (MuzzleMesh.IsValid() && MuzzleSocket != NAME_None && MuzzleMesh->DoesSocketExist(MuzzleSocket))
	{
		CurrentMuzzleLocation = MuzzleMesh->GetSocketLocation(MuzzleSocket);
	}

	if (Elapsed < OutboundDuration)
	{
		SetActorLocation(FMath::Lerp(Start, End, Elapsed / OutboundDuration));
	}
	else if (Elapsed < OutboundDuration + ReturnDuration)
	{
		const float ReturnAlpha = (Elapsed - OutboundDuration) / ReturnDuration;
		// 복귀 목적지도 발사 시점 스냅샷이 아니라 "현재" 총구 위치로 -> 실제로 손에 든 총 자리로 돌아와서 붙음.
		SetActorLocation(FMath::Lerp(End, CurrentMuzzleLocation, ReturnAlpha));
	}
	else
	{
		Destroy();
	}

	const float DistanceToStart = FVector::Dist(Cable->GetComponentLocation(), CurrentMuzzleLocation);
	Cable->CableLength = DistanceToStart;
	Cable->EndLocation = CurrentMuzzleLocation - Cable->GetComponentLocation();

	DrawDebugSphere(GetWorld(), CurrentMuzzleLocation, 15.f, 12, FColor::Red, false, 0.f);
	DrawDebugSphere(GetWorld(), GetActorLocation(), 15.f, 12, FColor::Green, false, 0.f);
}
