


#include "Player/HydrothermalVentZone.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Damageable.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

AHydrothermalVentZone::AHydrothermalVentZone()
{
	PrimaryActorTick.bCanEverTick = true;
	EffectArea = CreateDefaultSubobject<USphereComponent>(TEXT("EffectArea"));
	SetRootComponent(EffectArea);
	EffectArea->InitSphereRadius(400.f);
	EffectArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	EffectArea->SetGenerateOverlapEvents(true);

	VentSmokeVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VentSmokeVFX"));
	VentSmokeVFX->SetupAttachment(EffectArea);
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

	DrawVentDebugVisual();
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

void AHydrothermalVentZone::DrawVentDebugVisual() const
{
	const FVector Center = GetActorLocation();
	const float Radius = EffectArea->GetScaledSphereRadius();

	constexpr int32 NumColumns = 5;
	constexpr float ScrollSpeed = 300.f;
	constexpr float SegmentLength = 100.f;
	constexpr int32 NumArrowsPerColumn = 3;

	const float Time = GetWorld()->GetTimeSeconds();
	const float ScrollOffset = FMath::Fmod(Time * ScrollSpeed, SegmentLength);

	for (int32 Column = 0; Column < NumColumns; ++Column)
	{
		const float Angle = (2.f * PI / NumColumns) * Column;
		const float ColumnRadius = Radius * 0.3f;
		const FVector ColumnOffset(FMath::Cos(Angle) * ColumnRadius, FMath::Sin(Angle) * ColumnRadius, 0.f);
		const FVector ColumnBase = Center + ColumnOffset;

		for (int32 i = 0; i < NumArrowsPerColumn; ++i)
		{
			const float HeightOffset = (i * SegmentLength) + ScrollOffset;
			const FVector Start = ColumnBase + FVector(0.f, 0.f, HeightOffset);
			const FVector End = Start + FVector(0.f, 0.f, SegmentLength * 0.6f);

			DrawDebugDirectionalArrow(GetWorld(), Start, End, 30.f, FColor::Orange, false, -1.f, 0, 3.f);
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
}