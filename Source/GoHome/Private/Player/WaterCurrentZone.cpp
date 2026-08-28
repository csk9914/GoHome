

#include "Player/WaterCurrentZone.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/CarryWeightProvider.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

TMap<TWeakObjectPtr<ACharacter>, float> AWaterCurrentZone::GlobalOriginalBraking;
TMap<TWeakObjectPtr<ACharacter>, int32> AWaterCurrentZone::GlobalOverlapCount;

AWaterCurrentZone::AWaterCurrentZone()
{
	PrimaryActorTick.bCanEverTick = true;

	EffectArea = CreateDefaultSubobject<UBoxComponent>(TEXT("EffectArea"));
	SetRootComponent(EffectArea);
	EffectArea->SetBoxExtent(FVector(400.f, 400.f, 400.f));
	EffectArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	EffectArea->SetGenerateOverlapEvents(true);

	FlowVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlowVFX"));
	FlowVFX->SetupAttachment(EffectArea);
	FlowVFX->SetAbsolute(false, false, true);
	FlowVFX->bAutoActivate = true;
}

void AWaterCurrentZone::BeginPlay()
{
	Super::BeginPlay();

	EffectArea->OnComponentBeginOverlap.AddDynamic(this, &AWaterCurrentZone::OnEffectAreaBeginOverlap);
	EffectArea->OnComponentEndOverlap.AddDynamic(this, &AWaterCurrentZone::OnEffectAreaEndOverlap);
}

void AWaterCurrentZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyForceToOverlappingCharacters(DeltaTime);
}

void AWaterCurrentZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateFlowVFX();
}

void AWaterCurrentZone::OnEffectAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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
		int32& Count = GlobalOverlapCount.FindOrAdd(OtherCharacter);
		if (Count == 0)
		{
			GlobalOriginalBraking.Add(OtherCharacter, Movement->BrakingDecelerationSwimming);
		}
		Count++;
		Movement->BrakingDecelerationSwimming = ZoneBrakingDeceleration;
	}

	OnCharacterEnteredZone(OtherCharacter);
}

void AWaterCurrentZone::OnEffectAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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
		int32* Count = GlobalOverlapCount.Find(OtherCharacter);
		if (Count)
		{
			(*Count)--;
			if (*Count <= 0)
			{
				if (const float* Original = GlobalOriginalBraking.Find(OtherCharacter))
				{
					Movement->BrakingDecelerationSwimming = *Original;
					GlobalOriginalBraking.Remove(OtherCharacter);
				}
				GlobalOverlapCount.Remove(OtherCharacter);
			}
		}
	}

	OnCharacterExitedZone(OtherCharacter);
}

void AWaterCurrentZone::ApplyForceToOverlappingCharacters(float DeltaTime)
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

		const FVector Force = (ZoneType == EWaterCurrentZoneType::Whirlpool)
			? CalculateWhirlpoolForce(Character)
			: CalculateCurrentForce(Character);

		Movement->AddForce(Force * Movement->Mass);
	}
}

FVector AWaterCurrentZone::CalculateCurrentForce(const ACharacter* Character) const
{
	FVector Direction = FlowDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		// 방향이 다 0벡터면 액터가 바라보는 방향을 기본값으로 사용
		Direction = GetActorForwardVector();
	}

	return Direction * FlowStrength * GetWeightMultiplier(Character);
}

FVector AWaterCurrentZone::CalculateWhirlpoolForce(const ACharacter* Character) const
{
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	const FVector ToCenter = GetActorLocation() - Character->GetActorLocation();
	const FVector PullDirection = ToCenter.GetSafeNormal();

	// 위 벡터와 중심으로 향하는 방향을 외적하면 중심을 기준으로 도는 접선 방향이 나옴
	const FVector SpinDirection = FVector::CrossProduct(FVector::UpVector, PullDirection).GetSafeNormal();

	const float WeightMultiplier = GetWeightMultiplier(Character);
	const FVector PullForce = PullDirection * PullStrength * WeightMultiplier;
	const FVector SpinForce = SpinDirection * SpinStrength * WeightMultiplier;

	return PullForce + SpinForce;
}

float AWaterCurrentZone::GetWeightMultiplier(const AActor* OtherActor) const
{
	if (!bScaleForceByWeight || !OtherActor)
	{
		return 1.f;
	}

	// ICarryWeightProvider 인터페이스를 가진 컴포넌트를 찾아 무게를 조회
	for (UActorComponent* Component : OtherActor->GetComponents())
	{
		const ICarryWeightProvider* WeightProvider = Cast<ICarryWeightProvider>(Component);
		if (!WeightProvider)
		{
			continue;
		}

		const float CurrentWeight = WeightProvider->GetCurrentCarryWeight();
		return FMath::Clamp(1.f + CurrentWeight * WeightForceScale, 1.f, MaxWeightMultiplier);
	}

	return 1.f;
}

void AWaterCurrentZone::UpdateFlowVFX()
{
	if (!FlowVFX)
	{
		return;
	}

	UNiagaraSystem* TargetAsset = (ZoneType == EWaterCurrentZoneType::Whirlpool)
		? WhirlpoolVFXAsset
		: CurrentFlowVFXAsset;

	const bool bShouldShow = (TargetAsset != nullptr);
	FlowVFX->SetVisibility(bShouldShow);

	if (!bShouldShow)
	{
		return;
	}

	if (FlowVFX->GetAsset() != TargetAsset)
	{
		FlowVFX->SetAsset(TargetAsset);
	}

	FlowVFX->SetVariableVec3(FName("User.EffectScale"), EffectArea->GetScaledBoxExtent() / 400.f);
	FlowVFX->SetVariableFloat(FName("User.Radius"), WhirlpoolRadius);

	if (ZoneType == EWaterCurrentZoneType::Current)
	{
		FVector Direction = FlowDirection.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::ForwardVector;
		}
		FlowVFX->SetRelativeRotation(Direction.Rotation());
	}
	else
	{
		FlowVFX->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

