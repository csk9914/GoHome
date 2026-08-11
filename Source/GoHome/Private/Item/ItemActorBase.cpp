

#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "AI/NoiseType.h"
#include "TimerManager.h"

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
	DOREPLIFETIME(AItemActorBase, CurrentNoiseRadius);
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

		// 테스트용 디버깅 메시지(추후 삭제하면 된다.)
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 5.f, FColor::Red, 
				FString::Printf(TEXT("파손! 속도 =%.0f, BreakCount = %d, 현재가치 = %.0f"), 
					ImpactSpeed, BreakCount, GetCurrentValue()));
		}

	}
}



// NoiseType

void AItemActorBase::NotifyPickedUp()
{
	if (!HasAuthority() || !ItemData || !ItemData->bMakesNoise) return;

	if (CurrentNoiseRadius <= 0.f)
	{
		CurrentNoiseRadius = ItemData->BaseNoiseRadius;
	}

	UGoHomeNoiseLibrary::GenerateNoise(this, GetActorLocation(),
		CurrentNoiseRadius, ENoiseType::Medium, this);

	// 테스트용 디버깅 메시지(추후 삭제할 것).

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("소음 발생! 반경 = %.0f(픽업 직후)"),
			CurrentNoiseRadius));
	}


	// 여기까지 테스트용 디버깅 메시지.

	GetWorldTimerManager().SetTimer(NoiseGrowthTimerHandle, this,
		&AItemActorBase::GrowNoiseRadius, ItemData->NoiseGrowthIntervalSeconds, true);
}

void AItemActorBase::NotifyDropped()
{
	GetWorldTimerManager().ClearTimer(NoiseGrowthTimerHandle);
	// CurrentNoiseRadius는 그대로 둔다 -> 아이템을 드랍한 후 다시 주우면 멈췄던 지점에서 타이머가 이어서 증가.
}


void AItemActorBase::GrowNoiseRadius()
{
	if (!ItemData) return;

	CurrentNoiseRadius = FMath::Min(
		CurrentNoiseRadius + ItemData->NoiseRadiusGrowthPerInterval, 
		ItemData->MaxNoiseRadius);

	const ENoiseType Type = (CurrentNoiseRadius >= 1500.f) ? ENoiseType::Large : ENoiseType::Medium; 
	UGoHomeNoiseLibrary::GenerateNoise(this, GetActorLocation(), CurrentNoiseRadius, Type, this);

	// 테스트용 디버깅 메시지(추후 삭제할 것).
	if (GEngine)
	{
		const TCHAR* TypeStr = (Type == ENoiseType::Large) ? TEXT("Large") : TEXT("Medium");
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("소음 증가! 반경 = %.0f, Type = %s"), CurrentNoiseRadius, TypeStr));
	}
}
