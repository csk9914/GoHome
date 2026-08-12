#include "Player/HealthComponent.h"

#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// HP의 진짜 값은 서버가 정합니다.
	// BP에서 MaxHP를 150으로 바꿔도 시작 HP가 그 값으로 맞춰지게 합니다.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	HP = FMath::Max(0.f, MaxHP);
	BroadcastHPChanged();
}

void UHealthComponent::ApplyDamage_Implementation(float Amount, AActor* Instigator, FName DamageType)
{
	// HP 변경은 서버에서만 처리합니다. 클라이언트는 복제된 결과만 표시합니다.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 이미 죽었거나 의미 없는 데미지는 무시합니다.
	if (bIsDead || Amount <= 0.f)
	{
		return;
	}

	// HP가 0 아래로 내려가거나 MaxHP보다 커지지 않게 잠급니다.
	const float OldHP = HP;
	HP = FMath::Clamp(HP - Amount, 0.f, MaxHP);

	if (!FMath::IsNearlyEqual(HP, OldHP))
	{
		BroadcastHPChanged();
	}

	// 사망 처리는 한 번만 일어나야 합니다.
	if (HP <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
		OnDeathDynamic.Broadcast();
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
	BroadcastHPChanged();
}

void UHealthComponent::OnRep_IsDead()
{
	if (bIsDead)
	{
		// OnDeath는 서버 GameState 집계용입니다.
		// 클라이언트에서는 BP용 이벤트만 다시 알립니다.
		OnDeathDynamic.Broadcast();
	}
}

void UHealthComponent::BroadcastHPChanged()
{
	OnHPChanged.Broadcast(HP, MaxHP);
}
