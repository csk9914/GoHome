
#include "Item/DrillItemActor.h"
#include "Interaction/BreakableWallActor.h"
#include "Player/GoHomeCharacter.h"
#include "AI/NoiseType.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ADrillItemActor::ADrillItemActor()
{
	DrillVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DrillVFX"));
	DrillVFX->SetupAttachment(MeshComponent);
	DrillVFX->SetAutoActivate(false);

	DrillSFX = CreateDefaultSubobject<UAudioComponent>(TEXT("DrillSFX"));
	DrillSFX->SetupAttachment(MeshComponent);
	DrillSFX->SetAutoActivate(false);
}

void ADrillItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ChargesRemaining = BatteryCharges;
	}

	//드릴 팁 소켓이 있으면 코스메틱을 거기로 옮김(생성자는 BP 기본값 적용 전이라 여기서).
	if (MuzzleSocketName != NAME_None && MeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		DrillVFX->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, MuzzleSocketName);
		DrillSFX->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, MuzzleSocketName);
	}

}

void ADrillItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADrillItemActor, bIsDrilling);
	DOREPLIFETIME(ADrillItemActor, ChargesRemaining);
}


FVector ADrillItemActor::GetMuzzleLocation() const
{
	if (MuzzleSocketName != NAME_None && MeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return MeshComponent->GetSocketLocation(MuzzleSocketName);
	}

	return HoldingPawn ? HoldingPawn->GetActorLocation() + FVector::UpVector * 40.f : GetActorLocation();
}

FVector ADrillItemActor::GetAimDirection() const
{
	const AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn);

	if (!Character) return GetActorForwardVector();
	const FRotator AimRotation(Character->CurrentPitch, Character->GetActorRotation().Yaw, 0.f);
	return AimRotation.Vector();
}

ABreakableWallActor* ADrillItemActor::TraceForWall() const
{
	if (!HoldingPawn || !GetWorld()) return nullptr;

	const FVector Start = GetMuzzleLocation();
	const FVector End = Start + GetAimDirection() * TraceDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(HoldingPawn);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
		                                 FCollisionShape::MakeSphere(TraceRadius), Params))
	{
		ABreakableWallActor* Wall = Cast<ABreakableWallActor>(Hit.GetActor());
		if (Wall && !Wall->IsBroken())
		{
			return Wall;
		}
	}
	return nullptr;
}

void ADrillItemActor::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn || !CanUse()) return;

	ABreakableWallActor* Wall = TraceForWall();
	if (!Wall) return;

	LastUseTime = GetWorld()->GetTimeSeconds();
	DrillingTarget = Wall;
	DrillElapsed = 0.f;
	DrillNoiseTimer = 0.f;
	SetActorTickEnabled(true);

	bIsDrilling = true;
	OnRep_IsDrilling(); // 서버 자신은 OnRep이 안 뜨므로 수동 호출.

}

bool ADrillItemActor::CanUse() const
{
	if (DrillingTarget) return false; // 이미 드릴링 중.
	if (ChargesRemaining <= 0) return false; // 배터리 소진.
	return GetWorld() && (GetWorld()->GetTimeSeconds() - LastUseTime) >= UseCooldown;
}

void ADrillItemActor::Tick(float DeltaTime)
{
	if (DrillingTarget)
	{
		//  부모(AItemActorBase) Tick이 "물리 시뮬 아니면 Tick 끔"이라 회수 중엔 Super 건너뜀.
		if (!HasAuthority()) return;

		if (!IsValid(DrillingTarget) || DrillingTarget->IsBroken() || !HoldingPawn || !IsValid(HoldingPawn))
		{
			AbortDrill();
			return;
		}

		// 아직 그 벽을 조준 중인지 재확인 -> 벗어나면 중단.
		if (TraceForWall() != DrillingTarget)
		{
			AbortDrill();
			return;
		}

		DrillElapsed += DeltaTime;

		DrillNoiseTimer += DeltaTime;
		if (DrillNoiseTimer >= DrillNoiseInterval)
		{
			DrillNoiseTimer = 0.f;
			UGoHomeNoiseLibrary::GenerateNoise(this, GetMuzzleLocation(),
				DrillNoiseRadius, DrillNoiseType, HoldingPawn);
		}

		if (DrillElapsed >= DrillDuration * DrillingTarget->GetDrillTimeMultiplier())
		{
			DrillingTarget->ServerBreak(HoldingPawn, GetAimDirection());
			--ChargesRemaining;
			AbortDrill();
		}
		return;
	}
	Super::Tick(DeltaTime);
}

void ADrillItemActor::ServerDrop()
{
	if (DrillingTarget)
	{
		AbortDrill();
	}
	Super::ServerDrop();
}

void ADrillItemActor::AbortDrill()
{
	DrillingTarget = nullptr;
	DrillElapsed = 0.f;
	DrillNoiseTimer = 0.f;
	SetActorTickEnabled(false);

	bIsDrilling = false;
	OnRep_IsDrilling(); // 서버 수동 호출.
}

void ADrillItemActor::OnRep_IsDrilling()
{
	SetDrillCosmeticActive(bIsDrilling);
}

void ADrillItemActor::SetDrillCosmeticActive(bool bActive)
{
	if (bActive)
	{
		DrillVFX->Activate(true);
		DrillSFX->Play();
	}
	else
	{
		DrillVFX->Deactivate();
		DrillSFX->Stop();
	}
}