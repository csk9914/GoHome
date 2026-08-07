

#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"

AItemActorBase::AItemActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

bool AItemActorBase::CanInteract(APlayerController* PlayerController) const
{
	return !bIsBeingClaimed;
}

void AItemActorBase::OnInteract(APlayerController* PlayerController)
{
}

float AItemActorBase::GetTotalWeight() const
{
	return ItemData ? ItemData->Weight : 0.f;
}

void AItemActorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemActorBase, bIsBeingClaimed);
}
