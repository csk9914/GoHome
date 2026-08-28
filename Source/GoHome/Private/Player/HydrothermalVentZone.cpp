


#include "Player/HydrothermalVentZone.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Damageable.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

AHydrothermalVentZone::AHydrothermalVentZone()
{
	PrimaryActorTick.bCanEverTick = true;
	EffectArea = CreateDefaultSubobject<UBoxComponent>(TEXT("EffectArea"));
	SetRootComponent(EffectArea);
	EffectArea->SetBoxExtent(FVector(400.f, 400.f, 400.f));
	EffectArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	EffectArea->SetGenerateOverlapEvents(true);

	VentSmokeVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VentSmokeVFX"));
	VentSmokeVFX->SetupAttachment(EffectArea);
	VentSmokeVFX->SetAbsolute(false, false, true);
	VentSmokeVFX->bAutoActivate = true;
}

void AHydrothermalVentZone::BeginPlay()
{
	Super::BeginPlay();
	
	EffectArea->OnComponentBeginOverlap.AddDynamic(this, &AHydrothermalVentZone::OnEffectAreaBeginOverlap);
	EffectArea->OnComponentEndOverlap.AddDynamic(this, &AHydrothermalVentZone::OnEffectAreaEndOverlap);
}

void AHydrothermalVentZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyForceToOverlappingCharacters(DeltaTime);

	// 데미지는 서버 권위 판정이 걸린 상태 변화라서 서버에서만 계산
	if (HasAuthority())
	{
		ApplyDamageToOverlappingCharacters(DeltaTime);
	}
}

void AHydrothermalVentZone::OnEffectAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* OtherCharacter = Cast<ACharacter>(OtherActor);
	if (!OtherCharacter)
	{
		return;
	}

	if (!HasAuthority() && !OtherCharacter->IsLocallyControlled())
	{
		return;
	}

	OverlappingCharacters.AddUnique(OtherCharacter);

	if (UCharacterMovementComponent* Movement = OtherCharacter->GetCharacterMovement())
	{
		if (!OriginalBrakingDeceleration.Contains(OtherCharacter))
		{
			OriginalBrakingDeceleration.Add(OtherCharacter, Movement->BrakingDecelerationSwimming);
		}
		Movement->BrakingDecelerationSwimming = ZoneBrakingDeceleration;
	}

	OnCharacterEnteredZone(OtherCharacter);
}

void AHydrothermalVentZone::OnEffectAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACharacter* OtherCharacter = Cast<ACharacter>(OtherActor);
	if (!OtherCharacter)
	{
		return;
	}

	if (!HasAuthority() && !OtherCharacter->IsLocallyControlled())
	{
		return;
	}

	OverlappingCharacters.Remove(OtherCharacter);

	if (UCharacterMovementComponent* Movement = OtherCharacter->GetCharacterMovement())
	{
		if (const float* Original = OriginalBrakingDeceleration.Find(OtherCharacter))
		{
			Movement->BrakingDecelerationSwimming = *Original;
			OriginalBrakingDeceleration.Remove(OtherCharacter);
		}
	}

	OnCharacterExitedZone(OtherCharacter);
}

void AHydrothermalVentZone::ApplyForceToOverlappingCharacters(float DeltaTime)
{
	for (ACharacter* Character : OverlappingCharacters)
	{
		if (!IsValid(Character))
		{
			continue;
		}

		if (!HasAuthority() && !Character->IsLocallyControlled())
		{
			continue;
		}

		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		if (!Movement)
		{
			continue;
		}

		const FVector Force = FVector::UpVector * VentForceStrength;
		Movement->AddForce(Force * Movement->Mass);
	}
}

void AHydrothermalVentZone::ApplyDamageToOverlappingCharacters(float DeltaTime)
{
	const float DamageAmount = FMath::Max(0.f, DamagePerSecond) * DeltaTime;
	if (DamageAmount <= 0.f)
	{
		return;
	}

	for (ACharacter* Character : OverlappingCharacters)
	{
		if (!IsValid(Character))
		{
			continue;
		}

		// HP 컴포넌트 이름을 직접 모르고, "데미지를 받을 수 있는 대상"만 찾는다 (OxygenComponent::ApplySuffocationDamage와 동일한 방식)
		if (UActorComponent* DamageableComponent = Character->FindComponentByInterface(UDamageable::StaticClass()))
		{
			IDamageable::Execute_ApplyDamage(DamageableComponent, DamageAmount, this, DamageTypeName);
		}
	}
}

void AHydrothermalVentZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVentSmokeVFX();
}

void AHydrothermalVentZone::UpdateVentSmokeVFX()
{
	if (!VentSmokeVFX)
	{
		return;
	}

	VentSmokeVFX->SetVisibility(VentSmokeVFXAsset != nullptr);

	if (VentSmokeVFXAsset && VentSmokeVFX->GetAsset() != VentSmokeVFXAsset)
	{
		VentSmokeVFX->SetAsset(VentSmokeVFXAsset);
	}

	VentSmokeVFX->SetVariableVec3(FName("User.EffectScale"), EffectArea->GetScaledBoxExtent() / 400.f);
}