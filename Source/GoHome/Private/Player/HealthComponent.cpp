

#include "Player/HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHealthComponent::ApplyDamage_Implementation(float Amount, AActor* Instigator, FName DamageType)
{
	// 서버 아니면 무시
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 이미 죽었으면 /데미지 0 이하면 무시
	if (bIsDead || Amount <= 0.f)
	{
		return;
	}

	// HP를 데미지만큼 깎고 0 아래로 내려가지 않게 제한
	HP = FMath::Clamp(HP - Amount, 0.f, MaxHP);

	// 0 이하면 죽음
	if (HP <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, MaxHP);
	DOREPLIFETIME(UHealthComponent, HP);
	DOREPLIFETIME(UHealthComponent, bIsDead);
}

void UHealthComponent::OnRep_HP()
{
}
