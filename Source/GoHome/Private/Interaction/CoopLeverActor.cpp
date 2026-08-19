
#include "Interaction/CoopLeverActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"


ACoopLeverActor::ACoopLeverActor()
{	
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

}

bool ACoopLeverActor::CanInteract(APawn* InstigatorPawn) const
{
	return true;
}

void ACoopLeverActor::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority()) return;

	bIsActive = true;
	OnRep_IsActive(); // 서버 자신에게는 RepNotify가 자동 호출되지 않아 직접 호출.

	GetWorldTimerManager().SetTimer(DeactivateTimerHandle, this, 
		&ACoopLeverActor::Deactivate, ActiveDuration, false);
}

void ACoopLeverActor::OnRep_IsActive()
{
	OnLeverActiveChanged.Broadcast(bIsActive);
}

void ACoopLeverActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopLeverActor, bIsActive);
}

FText ACoopLeverActor::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("레버 작동"));
}

void ACoopLeverActor::Deactivate()
{
	bIsActive = false;
	OnRep_IsActive();
}