

#include "Item/UsableItemBase.h"
#include "Net/UnrealNetwork.h"

void AUsableItemBase::UpdateAttachment(APawn* OldHoldingPawn)
{
	Super::UpdateAttachment(OldHoldingPawn);

	if (HoldingPawn)
	{
		CancelDespawnTimer();
	}
}

void AUsableItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUsableItemBase, bDespawnImminent);
}

void AUsableItemBase::OnSettled()
{
	if (!HasAuthority() || HoldingPawn || CanUse()) return;

	const float WarningDelay = FMath::Max(0.f, DespawnDelayAfterSettled - DespawnWarningLeadTime);
	GetWorldTimerManager().SetTimer(DespawnTimerHandle, this, &AUsableItemBase::StartDespawnWarning, WarningDelay, false);
}

void AUsableItemBase::StartDespawnWarning()
{
	OnDespawnImminent();
	GetWorldTimerManager().SetTimer(DespawnTimerHandle, this, &AUsableItemBase::DespawnIfDepleted, DespawnWarningLeadTime, false);
}

void AUsableItemBase::DespawnIfDepleted()
{
	if (!CanUse())
	{
		Destroy();
	}
}

void AUsableItemBase::CancelDespawnTimer()
{
	if (GetWorldTimerManager().IsTimerActive(DespawnTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(DespawnTimerHandle);
		OnDespawnCanceled();
	}
}

void AUsableItemBase::OnDespawnImminent()
{
	bDespawnImminent = true;
	OnRep_DespawnImminent(); // 서버 자신에게는 RepNotify가 안 뜨므로 직접 호출.
}

void AUsableItemBase::OnDespawnCanceled()
{
	bDespawnImminent = false;
	OnRep_DespawnImminent();
}

void AUsableItemBase::OnRep_DespawnImminent()
{
	if (bDespawnImminent)
	{
		ToggleMeshFlicker();
	}
	else
	{
		GetWorldTimerManager().ClearTimer(MeshFlickerTimerHandle);
		MeshComponent->SetVisibility(true);
	}
}

void AUsableItemBase::ToggleMeshFlicker()
{
	MeshComponent->SetVisibility(!MeshComponent->IsVisible());
	GetWorldTimerManager().SetTimer(MeshFlickerTimerHandle, this,
		&AUsableItemBase::ToggleMeshFlicker, FMath::FRandRange(0.05f, 0.15f), false);
}