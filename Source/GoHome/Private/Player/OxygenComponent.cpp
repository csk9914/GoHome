

#include "Player/OxygenComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/Damageable.h"

UOxygenComponent::UOxygenComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

int32 UOxygenComponent::GetDisplayedOxygenPips() const
{
	/*
	Oxygen 14.8 -> 15칸
	Oxygen 14.0 -> 14칸
	Oxygen 0.0 -> 0칸
	*/
	return FMath::Clamp(FMath::CeilToInt(Oxygen), 0, FMath::CeilToInt(MaxOxygen));
}

float UOxygenComponent::GetOxygenPercent() const
{
	if (MaxOxygen <= 0.f)
	{
		return 0.f;
	}

	return FMath::Clamp(Oxygen / MaxOxygen, 0.f, 1.f);
}

void UOxygenComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Oxygen = MaxOxygen;
	}
}

void UOxygenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 서버에서만 산소를 줄인다.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (Oxygen > 0.f)
	{
		const float DrainRate = CalculateOxygenDrainRate();
		Oxygen = FMath::Clamp(Oxygen - DrainRate * DeltaTime, 0.f, MaxOxygen);
	}

	if (Oxygen <= 0.f)
	{
		const float DamageAmount = SuffocationDamagePerSecond * DeltaTime;

		if (DamageAmount <= 0.f)
		{
			return;
		}

		// 여기서 IDamageable 찾아서 ApplyDamage 호출
		// Find an IDamageable component and apply suffocation damage.
		const TSet<UActorComponent*>& Components = GetOwner()->GetComponents();

		for (UActorComponent* Component : Components)
		{
			if (!Component)
			{
				continue;
			}

			if (Component->GetClass()->ImplementsInterface(UDamageable::StaticClass()))
			{
				IDamageable::Execute_ApplyDamage(Component, DamageAmount, GetOwner(), FName(TEXT("Suffocation")));
				return;
			}
		}
	}
}

void UOxygenComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOxygenComponent, MaxOxygen);
	DOREPLIFETIME(UOxygenComponent, Oxygen);
}

float UOxygenComponent::CalculateOxygenDrainRate() const
{
	if (TargetOxygenDuration <= 0.f)
	{
		return 0.f;
	}

	return MaxOxygen / TargetOxygenDuration;
}

void UOxygenComponent::OnRep_Oxygen()
{
}
