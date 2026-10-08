#include "Item/RecoveryItemBase.h"

#include "Components/PointLightComponent.h"
#include "GameFramework/Pawn.h"
#include "Interaction/InventoryComponent.h"
#include "Player/HealthComponent.h"
#include "Player/OxygenComponent.h"

ARecoveryItemBase::ARecoveryItemBase()
{
	PickupGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("PickupGlow"));
	PickupGlow->SetupAttachment(MeshComponent);
	PickupGlow->SetCastShadows(false);
	PickupGlow->SetIntensity(PickupGlowIntensity);
	PickupGlow->SetAttenuationRadius(PickupGlowRadius);
}

void ARecoveryItemBase::BeginPlay()
{
	Super::BeginPlay();

	if (!PickupGlow)
	{
		return;
	}

	PickupGlow->SetVisibility(bEnablePickupGlow, true);
	PickupGlow->SetIntensity(FMath::Max(0.f, PickupGlowIntensity));
	PickupGlow->SetAttenuationRadius(FMath::Max(0.f, PickupGlowRadius));
	PickupGlow->SetLightColor(RecoveryType == ERecoveryItemType::Oxygen
		? OxygenGlowColor
		: HealthGlowColor);
}

bool ARecoveryItemBase::CanInteract(APawn* InstigatorPawn) const
{
	return Super::CanInteract(InstigatorPawn)
		&& !HoldingPawn
		&& CanRestorePawn(InstigatorPawn);
}

void ARecoveryItemBase::OnInteract(APawn* InstigatorPawn)
{
	// 맵 회복 아이템은 E를 누르면 인벤토리를 차지하지 않고 즉시 사용한다.
	if (!HasAuthority() || !CanInteract(InstigatorPawn))
	{
		return;
	}

	ConsumeForPawn(InstigatorPawn, false);
}

bool ARecoveryItemBase::CanUse() const
{
	return HoldingPawn && CanRestorePawn(HoldingPawn);
}

void ARecoveryItemBase::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn || !CanUse())
	{
		return;
	}

	ConsumeForPawn(HoldingPawn, true);
}

FText ARecoveryItemBase::GetInteractionPromptText_Implementation() const
{
	const FText ActionText = RecoveryType == ERecoveryItemType::Oxygen
		? NSLOCTEXT("RecoveryItem", "OxygenAction", "산소 회복")
		: NSLOCTEXT("RecoveryItem", "HealthAction", "HP 회복");

	return ActionText;
}

bool ARecoveryItemBase::CanRestorePawn(const APawn* TargetPawn) const
{
	if (!IsValid(TargetPawn) || RecoveryAmount <= 0.f)
	{
		return false;
	}

	if (RecoveryType == ERecoveryItemType::Oxygen)
	{
		const UOxygenComponent* Oxygen = TargetPawn->FindComponentByClass<UOxygenComponent>();
		return Oxygen
			&& Oxygen->GetOxygen() + KINDA_SMALL_NUMBER < Oxygen->GetMaxOxygen();
	}

	const UHealthComponent* Health = TargetPawn->FindComponentByClass<UHealthComponent>();
	return Health
		&& !Health->IsDead()
		&& Health->GetHP() + KINDA_SMALL_NUMBER < Health->GetMaxHP();
}

bool ARecoveryItemBase::ApplyRecovery(APawn* TargetPawn)
{
	if (!HasAuthority() || !CanRestorePawn(TargetPawn))
	{
		return false;
	}

	if (RecoveryType == ERecoveryItemType::Oxygen)
	{
		if (UOxygenComponent* Oxygen = TargetPawn->FindComponentByClass<UOxygenComponent>())
		{
			return Oxygen->RestoreOxygen(RecoveryAmount) > KINDA_SMALL_NUMBER;
		}
		return false;
	}

	if (UHealthComponent* Health = TargetPawn->FindComponentByClass<UHealthComponent>())
	{
		return Health->RestoreHealth(RecoveryAmount) > KINDA_SMALL_NUMBER;
	}

	return false;
}

bool ARecoveryItemBase::ConsumeForPawn(APawn* TargetPawn, bool bRequireInventory)
{
	if (!HasAuthority() || !TargetPawn)
	{
		return false;
	}

	UInventoryComponent* Inventory = TargetPawn->FindComponentByClass<UInventoryComponent>();
	if (bRequireInventory && (!Inventory || Inventory->FindSlotIndexOf(this) == INDEX_NONE))
	{
		return false;
	}

	if (!bRequireInventory)
	{
		bIsBeingClaimed = true;
	}

	if (!ApplyRecovery(TargetPawn))
	{
		if (!bRequireInventory)
		{
			bIsBeingClaimed = false;
		}
		return false;
	}

	if (Inventory && Inventory->FindSlotIndexOf(this) != INDEX_NONE)
	{
		Inventory->RemoveItem(this);
	}

	Destroy();
	return true;
}
