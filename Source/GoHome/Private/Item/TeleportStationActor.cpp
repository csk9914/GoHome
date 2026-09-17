


#include "Item/TeleportStationActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ATeleportStationActor::ATeleportStationActor()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	// 정적 배치물이라 위치 복제는 불필요 - 충전 상태만 복제
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	TeleportTarget = CreateDefaultSubobject<USceneComponent>(TEXT("TeleportTarget"));
	TeleportTarget->SetupAttachment(SceneRoot);
	TeleportTarget->SetRelativeLocation(FVector(150.f, 0.f, 0.f));
}

FTransform ATeleportStationActor::GetTeleportTransform() const
{
	return TeleportTarget ? TeleportTarget->GetComponentTransform() : GetActorTransform();
}

void ATeleportStationActor::SetCharging(bool bNewCharging, float InDuration)
{
	if (!HasAuthority()) return;
	if (bIsCharging == bNewCharging) return;

	ChargeDuration = bNewCharging ? InDuration : 0.f;
	bIsCharging = bNewCharging;
	OnRep_IsCharging(); // 서버 자신에게는 RepNotify가 안 뜨므로 직접 호출
}

void ATeleportStationActor::Multicast_PlayArrivalBurst_Implementation()
{
	OnArrivalBurst();
}

void ATeleportStationActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATeleportStationActor, bIsCharging);
	DOREPLIFETIME(ATeleportStationActor, ChargeDuration);
}

void ATeleportStationActor::OnRep_IsCharging()
{
	OnChargingChanged(bIsCharging);
}


