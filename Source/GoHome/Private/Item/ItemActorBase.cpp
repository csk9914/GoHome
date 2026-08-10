

#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"

AItemActorBase::AItemActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetNotifyRigidBodyCollision(true);
}

bool AItemActorBase::CanInteract(APawn* InstigatorPawn) const
{
	return !bIsBeingClaimed;
}

void AItemActorBase::OnInteract(APawn* InstigatorPawn)
{
}

float AItemActorBase::GetTotalWeight() const
{
	return ItemData ? ItemData->Weight : 0.f;
}

float AItemActorBase::GetCurrentValue() const
{
	if (!ItemData) return 0.f;

	const float PenaltyRatio = ItemData->BreakValuePenaltyPercent * BreakCount;
	const float ValueRatio = FMath::Max(1.f - PenaltyRatio, ItemData->MinValuePercent);
	return ItemData->Value * ValueRatio;
}

void AItemActorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemActorBase, bIsBeingClaimed);
	DOREPLIFETIME(AItemActorBase, BreakCount);
}

void AItemActorBase::NotifyHit(UPrimitiveComponent* MyComp, 
	AActor* Other, 
	UPrimitiveComponent* OtherComp, 
	bool bSelfMoved, 
	FVector HitLocation, 
	FVector HitNormal, 
	FVector NormalImpulse, 
	const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (!HasAuthority() || !ItemData || !ItemData->bCanBreak) return;
	if (BreakCount >= ItemData->MaxBreakCount) return;

	const float ImpactSpeed = GetVelocity().Size();
	if (ImpactSpeed >= ItemData->BreakVelocityThreshold)
	{
		++BreakCount;
	}
}
