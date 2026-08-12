#include "Player/CarryWeightComponent.h"

UCarryWeightComponent::UCarryWeightComponent()
{
	// InventoryComponent를 직접 호출하지 않으므로, 짧은 주기로 무게 제공자들을 다시 확인한다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(true);
}


void UCarryWeightComponent::BeginPlay()
{
	Super::BeginPlay();
	FindWeightProviderComponents();
	RefreshCurrentCarryWeight();

}


// 다른 시스템은 이 저장값만 읽는다. 실제 합산은 RefreshCurrentCarryWeight()에서 담당한다.
float UCarryWeightComponent::GetCurrentCarryWeight() const
{
	return CurrentCarryWeight;
}

// 기본 한도 + 업그레이드 + 임시 보정값을 합쳐 최종 최대 무게를 만든다.
float UCarryWeightComponent::GetMaxCarryWeight() const
{
	const float MaxCarryWeight =
		BaseMaxCarryWeight
		+ MaxCarryWeightBonus
		+ TemporaryMaxCarryWeightModifier;

	return FMath::Max(0.f, MaxCarryWeight);
}

// 현재 무게가 최대 무게를 넘은 만큼만 반환한다. 넘지 않으면 0이다.
float UCarryWeightComponent::GetOverweightAmount() const
{
	return FMath::Max(0.f, GetCurrentCarryWeight() - GetMaxCarryWeight());
}

void UCarryWeightComponent::SetMaxCarryWeightBonus(float NewBonus)
{
	MaxCarryWeightBonus = FMath::Max(0.f, NewBonus);
}

void UCarryWeightComponent::SetTemporaryMaxCarryWeightModifier(float NewModifier)
{
	TemporaryMaxCarryWeightModifier = NewModifier;
}

void UCarryWeightComponent::FindWeightProviderComponents()
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
		if (!OwnerComponent || OwnerComponent == this)
		{
			continue;
		}

		IWeightProvider* WeightProvider = Cast<IWeightProvider>(OwnerComponent);
		if (!WeightProvider)
		{
			continue;
		}

		// TScriptInterface는 UObject와 Interface 포인터를 함께 보관한다.
		TScriptInterface<IWeightProvider> WeightProviderInterface;
		WeightProviderInterface.SetObject(OwnerComponent);
		WeightProviderInterface.SetInterface(WeightProvider);

		CachedWeightProviders.Add(WeightProviderInterface);
	}
}

float UCarryWeightComponent::GetTotalWeightFromProviders() const
{
	// BeginPlay 순서 차이로 아직 제공자를 못 찾았을 수 있으므로 한 번 더 찾는다.
	if (CachedWeightProviders.Num() == 0)
	{
		// const 함수 안에서 캐시만 갱신한다. 게임 상태 값은 바꾸지 않는다.
		const_cast<UCarryWeightComponent*>(this)->FindWeightProviderComponents();
	}

	float TotalWeight = 0.f;

	for (const TScriptInterface<IWeightProvider>& CachedWeightProvider : CachedWeightProviders)
	{
		const UObject* WeightProviderObject = CachedWeightProvider.GetObject();
		const IWeightProvider* WeightProvider = CachedWeightProvider.GetInterface();

		if (!IsValid(WeightProviderObject) || !WeightProvider)
		{
			continue;
		}

		TotalWeight += FMath::Max(0.f, WeightProvider->GetTotalWeight());
	}

	return TotalWeight;
}

void UCarryWeightComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshCurrentCarryWeight();
}

void UCarryWeightComponent::RefreshCurrentCarryWeight()
{
	const float NewCarryWeight = GetTotalWeightFromProviders();

	// 값이 같으면 저장값을 다시 쓰지 않는다. 나중에 변경 이벤트를 붙여도 중복 방송을 막기 쉽다.
	if (FMath::IsNearlyEqual(CurrentCarryWeight, NewCarryWeight))
	{
		return;
	}

	CurrentCarryWeight = NewCarryWeight;
}
