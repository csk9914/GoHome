
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

	// EndLocation 직접 계산 방식은 좌표계 해석이 계속 어긋나서 신뢰 불가 -> 엔진 내장 기능으로 대체.
	// Cable의 자유단을 총 메쉬의 소켓에 직접 붙여서, 엔진이 알아서 매 프레임 그 위치를 추적하게 함.
	if (InMuzzleMesh)
	{
		Cable->SetAttachEndTo(InMuzzleMesh->GetOwner(), InMuzzleMesh->GetFName(), InMuzzleSocket);
	}
}

void AHarpoonCosmeticActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Elapsed += DeltaTime;

	FVector RawMuzzleLocation = Start;
	if (MuzzleMesh.IsValid() && MuzzleSocket != NAME_None && MuzzleMesh->DoesSocketExist(MuzzleSocket))
	{
		RawMuzzleLocation = MuzzleMesh->GetSocketLocation(MuzzleSocket);
	}

	if (!bMuzzleLocationInitialized)
	{
		SmoothedMuzzleLocation = RawMuzzleLocation;
		bMuzzleLocationInitialized = true;
	}

	else
	{
		SmoothedMuzzleLocation = FMath::VInterpTo(SmoothedMuzzleLocation, RawMuzzleLocation, DeltaTime, 15.f);
	}

	if (Elapsed < OutboundDuration)
	{
		SetActorLocation(FMath::Lerp(Start, End, Elapsed / OutboundDuration));
	}

	else if (Elapsed < OutboundDuration + ReturnDuration)
	{
		const float ReturnAlpha = (Elapsed - OutboundDuration) / ReturnDuration;
		SetActorLocation(FMath::Lerp(End, SmoothedMuzzleLocation, ReturnAlpha));
	}
	
	else
	{
		Destroy();
	}

	// Cable->EndLocation / CableLength 수동 계산 삭제 -> SetAttachEndTo가 매 프레임 자동 갱신.
}
