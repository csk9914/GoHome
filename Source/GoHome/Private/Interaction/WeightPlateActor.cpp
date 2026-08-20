
#include "Interaction/WeightPlateActor.h"
#include "Interaction/WeightProvider.h"
#include "Core/DockingDoorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"


AWeightPlateActor::AWeightPlateActor()
{
 
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(MeshComponent);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
}


void AWeightPlateActor::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	if (TargetDoorActor)
	{
		TargetDoor = TargetDoorActor->FindComponentByClass<UDockingDoorComponent>();
	}

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AWeightPlateActor::OnTriggerBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AWeightPlateActor::OnTriggerEndOverlap);

	GetWorldTimerManager().SetTimer(CheckTimerHandle, this,
		&AWeightPlateActor::EvaluatePlateState, CheckInterval, true);
}

void AWeightPlateActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		OverlappingPawns.Add(Pawn);
	}
}

void AWeightPlateActor::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		OverlappingPawns.Remove(Pawn);
	}
}

float AWeightPlateActor::CalculateCurrentWeight() const
{
	float TotalWeight = 0.f;

	for (const TObjectPtr<APawn>& PawnPtr : OverlappingPawns)
	{
		APawn* Pawn = PawnPtr.Get();
		if (!Pawn) continue;

		for (UActorComponent* Component : Pawn->GetComponents())
		{
			if (IWeightProvider* WeightProvider = Cast<IWeightProvider>(Component))
			{
				TotalWeight += FMath::Max(0.f, WeightProvider->GetTotalWeight());
				break; // 보통 InventoryComponent 하나뿐이라 찾으면 바로 다음 Pawn으로.
			}
		}
	}
	return TotalWeight;
}

void AWeightPlateActor::EvaluatePlateState()
{
	const float NewWeight = CalculateCurrentWeight();
	if (!FMath::IsNearlyEqual(CachedCurrentWeight, NewWeight))
	{
		CachedCurrentWeight = NewWeight;
		OnRep_CurrentWeight(); // 서버 자신에게는 RepNotify가 자동 호출되지 않아 직접 호출.
	}

	if (bIsTriggered || !TargetDoor) return;

	if (CachedCurrentWeight >= WeightThreshold)
	{
		bIsTriggered = true;
		TargetDoor->SetOpen(true);
		GetWorldTimerManager().SetTimer(CloseTimerHandle, this, 
			&AWeightPlateActor::CloseAfterDuration, OpenDuration, false);
	}
}

void AWeightPlateActor::CloseAfterDuration()
{
	bIsTriggered = false;

	if (TargetDoor)
	{
		TargetDoor->SetOpen(false);
	}
}

void AWeightPlateActor::OnRep_CurrentWeight()
{
	OnProgressChanged.Broadcast(GetFilledPipCount(), GetTotalPipCount());
}

int32 AWeightPlateActor::GetFilledPipCount() const
{
	return FMath::Clamp(FMath::FloorToInt(CachedCurrentWeight), 0, GetTotalPipCount());
}

int32 AWeightPlateActor::GetTotalPipCount() const
{
	return FMath::CeilToInt(WeightThreshold);
}

void AWeightPlateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeightPlateActor, CachedCurrentWeight);
}