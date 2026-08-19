

#include "Player/WaterCurrentZone.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/CarryWeightProvider.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

AWaterCurrentZone::AWaterCurrentZone()
{
	PrimaryActorTick.bCanEverTick = true;

	EffectArea = CreateDefaultSubobject<USphereComponent>(TEXT("EffectArea"));
	SetRootComponent(EffectArea);
	EffectArea->InitSphereRadius(400.f);
	EffectArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	EffectArea->SetGenerateOverlapEvents(true);

	FlowIndicator = CreateDefaultSubobject<UArrowComponent>(TEXT("FlowIndicator"));
	FlowIndicator->SetupAttachment(EffectArea);
	FlowIndicator->SetArrowColor(FLinearColor::Blue);
	FlowIndicator->ArrowSize = 2.f;
	FlowIndicator->SetHiddenInGame(true);

	FlowVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlowVFX"));
	FlowVFX->SetupAttachment(EffectArea);
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

	if (ZoneType == EWaterCurrentZoneType::Whirlpool)
	{
		DrawWhirlpoolDebugVisual();
	}
	else
	{
		DrawFlowDebugVisual();
	}
}

void AWaterCurrentZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateFlowIndicator();
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
		OriginalBrakingDeceleration.Add(OtherCharacter, Movement->BrakingDecelerationSwimming);
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
		if (const float* Original = OriginalBrakingDeceleration.Find(OtherCharacter))
		{
			Movement->BrakingDecelerationSwimming = *Original;
			OriginalBrakingDeceleration.Remove(OtherCharacter);
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
		// 방향을 안 정했으면 액터가 바라보는 방향으로 흘려보냄
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

	// 위쪽 벡터와 중심으로 향하는 방향을 외적하면 중심을 축으로 도는 접선 방향이 나옴
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

	// ICarryWeightProvider 계약으로만 현재 무게를 읽음
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

void AWaterCurrentZone::UpdateFlowIndicator()
{
	if (!FlowIndicator)
	{
		return;
	}

	const bool bShouldShow = (ZoneType == EWaterCurrentZoneType::Current);
	FlowIndicator->SetVisibility(bShouldShow);

	if (bShouldShow)
	{
		FVector Direction = FlowDirection.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::ForwardVector;
		}
		FlowIndicator->SetRelativeRotation(Direction.Rotation());
	}
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

void AWaterCurrentZone::DrawFlowDebugVisual() const
{
	FVector Direction = FlowDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	const float Radius = EffectArea->GetScaledSphereRadius();
	constexpr float ScrollSpeed = 200.f;
	constexpr float SegmentLength = 150.f;
	constexpr int32 NumArrows = 4;
	constexpr float HeightOffset = 80.f;

	const float Time = GetWorld()->GetTimeSeconds();
	const float ScrollOffset = FMath::Fmod(Time * ScrollSpeed, SegmentLength);
	const FVector VerticalOffset(0.f, 0.f, HeightOffset);

	for (int32 i = 0; i < NumArrows; ++i)
	{
		const float DistanceAlongFlow = (i * SegmentLength) + ScrollOffset - Radius;
		const FVector Start = GetActorLocation() + Direction * DistanceAlongFlow + VerticalOffset;
		const FVector End = Start + Direction * (SegmentLength * 0.6f);

		DrawDebugDirectionalArrow(GetWorld(), Start, End, 40.f, FColor::Cyan, false, -1.f, 0, 4.f);
	}
}

void AWaterCurrentZone::DrawWhirlpoolDebugVisual() const
{
	const FVector Center = GetActorLocation();
	const float Radius = EffectArea->GetScaledSphereRadius();
	const float Time = GetWorld()->GetTimeSeconds();

	constexpr int32 NumArms = 3;
	constexpr int32 PointsPerArm = 6;
	constexpr float SpinSpeed = 2.f;
	constexpr float SpiralTwist = 3.f;
	constexpr float InwardSpeed = 0.4f;

	for (int32 Arm = 0; Arm < NumArms; ++Arm)
	{
		const float ArmPhase = (2.f * PI / NumArms) * Arm;

		for (int32 Point = 0; Point < PointsPerArm; ++Point)
		{
			const float PointPhase = static_cast<float>(Point) / PointsPerArm;
			const float Progress = FMath::Fmod(Time * InwardSpeed + PointPhase, 1.f);

			const float CurrentRadius = Radius * (1.f - Progress);
			const float Angle = ArmPhase - Time * SpinSpeed - Progress * SpiralTwist;

			const FVector Offset(FMath::Cos(Angle) * CurrentRadius, FMath::Sin(Angle) * CurrentRadius, 0.f);
			const FVector PointLocation = Center + Offset;

			DrawDebugSphere(GetWorld(), PointLocation, 15.f, 8, FColor::Magenta, false, -1.f, 0, 2.f);
		}
	}
}

