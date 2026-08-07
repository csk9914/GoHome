

#include "Player/OxygenComponent.h"
#include "Net/UnrealNetwork.h"

UOxygenComponent::UOxygenComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UOxygenComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UOxygenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UOxygenComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOxygenComponent, MaxOxygen);
	DOREPLIFETIME(UOxygenComponent, Oxygen);
}

void UOxygenComponent::OnRep_Oxygen()
{
}
