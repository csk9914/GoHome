#include "Player/BreathBubbleComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UBreathBubbleComponent::UBreathBubbleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 순수 연출이므로 네트워크로 동기화하지 않는다.
	SetIsReplicatedByDefault(false);
}

void UBreathBubbleComponent::BeginPlay()
{
	Super::BeginPlay();

	// 전용 서버는 그릴 화면이 없으므로 아예 틱을 돌리지 않는다.
	const AActor* Owner = GetOwner();
	if (Owner && Owner->GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}

	CachedMesh = ResolveMesh();
	NextBreathInterval = PickNextInterval();
}

void UBreathBubbleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!BreathBubbleFX)
	{
		return;
	}

	if (NextBreathInterval <= 0.f)
	{
		NextBreathInterval = PickNextInterval();
	}

	BreathTimer += DeltaTime;
	if (BreathTimer >= NextBreathInterval)
	{
		TriggerBreath();
	}
}

void UBreathBubbleComponent::TriggerBreath()
{
	// 이펙트 재생에 실패하더라도 타이머는 먼저 돌려놓는다.
	// 그래야 에셋이 비어 있을 때 매 프레임 재시도하지 않는다.
	BreathTimer = 0.f;
	NextBreathInterval = PickNextInterval();

	if (!BreathBubbleFX)
	{
		return;
	}

	if (!CachedMesh)
	{
		CachedMesh = ResolveMesh();
	}

	if (!CachedMesh)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(
		BreathBubbleFX,
		CachedMesh,
		BreathSocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		/*bAutoDestroy=*/ true);
}

float UBreathBubbleComponent::PickNextInterval() const
{
	// 에디터에서 Min > Max로 잘못 넣어도 동작하도록 정렬해서 쓴다.
	const float LowerBound = FMath::Min(BreathIntervalMin, BreathIntervalMax);
	const float UpperBound = FMath::Max(BreathIntervalMin, BreathIntervalMax);

	float Interval = FMath::FRandRange(LowerBound, UpperBound);

	// 움직이는 중이면 숨이 가빠진다.
	if (const AActor* Owner = GetOwner())
	{
		if (Owner->GetVelocity().Size() > MoveSpeedThreshold)
		{
			Interval *= MovingIntervalScale;
		}
	}

	return FMath::Max(Interval, MinBreathInterval);
}

USkeletalMeshComponent* UBreathBubbleComponent::ResolveMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// 캐릭터라면 본체 메시를 우선 사용한다.
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh())
		{
			return CharacterMesh;
		}
	}

	// 캐릭터가 아니거나 본체 메시가 없으면, 붙어 있는 스켈레탈 메시를 찾아 쓴다.
	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}
