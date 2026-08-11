#include "Player/OxygenComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/Damageable.h"
#include "Interaction/WeightProvider.h"

UOxygenComponent::UOxygenComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

float UOxygenComponent::GetOxygen() const
{
	return Oxygen;
}

float UOxygenComponent::GetMaxOxygen() const
{
	return MaxOxygen;
}

int32 UOxygenComponent::GetDisplayedOxygenPips() const
{
	/*
	Oxygen 14.8 -> 15칸
	Oxygen 14.0 -> 14칸
	Oxygen 0.0 -> 0칸
	*/
	const int32 MaxPips = FMath::Max(0, FMath::CeilToInt(MaxOxygen));
	return FMath::Clamp(FMath::CeilToInt(Oxygen), 0, MaxPips);
}

float UOxygenComponent::GetOxygenPercent() const
{
	if (MaxOxygen <= 0.f)
	{
		return 0.f;
	}

	return FMath::Clamp(Oxygen / MaxOxygen, 0.f, 1.f);
}

bool UOxygenComponent::IsInSafeZone() const
{
	return bInSafeZone;
}

void UOxygenComponent::SetInSafeZone(bool bNewInSafeZone)
{
	if (!HasOwnerAuthority() || bInSafeZone == bNewInSafeZone)
	{
		return;
	}

	bInSafeZone = bNewInSafeZone;
	OnSafeZoneChanged.Broadcast(bInSafeZone);
}

void UOxygenComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!HasOwnerAuthority())
	{
		SetComponentTickEnabled(false);
		return;
	}

	FindWeightProviderComponents();
	SetOxygen(MaxOxygen);
}

void UOxygenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HasOwnerAuthority() || DeltaTime <= 0.f)
	{
		return;
	}

	UpdateOxygen(DeltaTime);
}

void UOxygenComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UOxygenComponent, MaxOxygen);
	DOREPLIFETIME(UOxygenComponent, Oxygen);
	DOREPLIFETIME(UOxygenComponent, bInSafeZone);
}

bool UOxygenComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void UOxygenComponent::FindWeightProviderComponents()
{
	CachedWeightProviders.Reset();

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const TSet<UActorComponent*>& OwnerComponents = OwnerActor->GetComponents();
	for (UActorComponent* OwnerComponent : OwnerComponents)
	{
		// 자기 자신이거나 비어 있는 컴포넌트는 무게 계산 대상이 아니다.
		if (!OwnerComponent || OwnerComponent == this)
		{
			continue;
		}

		IWeightProvider* WeightProvider = Cast<IWeightProvider>(OwnerComponent);
		if (!WeightProvider)
		{
			continue;
		}

		// TScriptInterface는 UObject 포인터와 Interface 포인터를 함께 들고 있다.
		// 그래서 "어떤 오브젝트인지"와 "어떤 인터페이스로 부를지"를 둘 다 넣어준다.
		TScriptInterface<IWeightProvider> WeightProviderInterface;
		WeightProviderInterface.SetObject(OwnerComponent);
		WeightProviderInterface.SetInterface(WeightProvider);

		CachedWeightProviders.Add(WeightProviderInterface);
	}
}

void UOxygenComponent::UpdateOxygen(float DeltaTime)
{
	if (bInSafeZone)
	{
		RecoverOxygen(DeltaTime);
		return;
	}

	DrainOxygen(DeltaTime);

	if (Oxygen <= 0.f)
	{
		ApplySuffocationDamage(DeltaTime);
	}
}

void UOxygenComponent::RecoverOxygen(float DeltaTime)
{
	const float RecoveryAmount = FMath::Max(0.f, SafeZoneRecoveryRate) * DeltaTime;
	SetOxygen(Oxygen + RecoveryAmount);
}

void UOxygenComponent::DrainOxygen(float DeltaTime)
{
	if (Oxygen <= 0.f)
	{
		return;
	}

	const float DrainAmount = CalculateOxygenDrainRate() * DeltaTime;
	SetOxygen(Oxygen - DrainAmount);
}

void UOxygenComponent::ApplySuffocationDamage(float DeltaTime)
{
	const float DamageAmount = FMath::Max(0.f, SuffocationDamagePerSecond) * DeltaTime;
	if (DamageAmount <= 0.f)
	{
		return;
	}

	// HP 컴포넌트 이름을 직접 모르고, "데미지를 받을 수 있는 대상"만 찾는다.
	if (UActorComponent* DamageableComponent = GetOwner()->FindComponentByInterface(UDamageable::StaticClass()))
	{
		IDamageable::Execute_ApplyDamage(DamageableComponent, DamageAmount, GetOwner(), FName(TEXT("Suffocation")));
	}
}

void UOxygenComponent::SetOxygen(float NewOxygen)
{
	const float SafeMaxOxygen = FMath::Max(0.f, MaxOxygen);
	const float ClampedOxygen = FMath::Clamp(NewOxygen, 0.f, SafeMaxOxygen);

	if (FMath::IsNearlyEqual(Oxygen, ClampedOxygen))
	{
		return;
	}

	Oxygen = ClampedOxygen;
	OnOxygenChanged.Broadcast(Oxygen, SafeMaxOxygen);
}

float UOxygenComponent::CalculateOxygenDrainRate() const
{
	if (MaxOxygen <= 0.f || TargetOxygenDuration <= 0.f)
	{
		return 0.f;
	}

	const float BaseDrainRate = MaxOxygen / TargetOxygenDuration;
	const float WeightDrainRate = GetCachedTotalWeight() * FMath::Max(0.f, WeightDrainMultiplier);

	return FMath::Max(0.f, BaseDrainRate + WeightDrainRate);
}

float UOxygenComponent::GetCachedTotalWeight() const
{
	float TotalWeight = 0.f;

	for (const TScriptInterface<IWeightProvider>& CachedWeightProvider : CachedWeightProviders)
	{
		const UObject* WeightProviderObject = CachedWeightProvider.GetObject();
		const IWeightProvider* WeightProvider = CachedWeightProvider.GetInterface();

		// 캐시된 오브젝트가 삭제됐거나 인터페이스가 없으면 무시한다.
		if (!IsValid(WeightProviderObject) || !WeightProvider)
		{
			continue;
		}

		TotalWeight += FMath::Max(0.f, WeightProvider->GetTotalWeight());
	}

	return TotalWeight;
}

void UOxygenComponent::OnRep_Oxygen()
{
	OnOxygenChanged.Broadcast(Oxygen, MaxOxygen);
}

void UOxygenComponent::OnRep_InSafeZone()
{
	OnSafeZoneChanged.Broadcast(bInSafeZone);
}
