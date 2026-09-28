#include "Item/HammerItemActor.h"
#include "Interaction/BreakableWallActor.h"
#include "Player/GoHomeCharacter.h"
#include "TimerManager.h"

FVector AHammerItemActor::GetImpactLocation() const
{
	if (ImpactSocketName != NAME_None && MeshComponent->DoesSocketExist(ImpactSocketName))
	{
		return MeshComponent->GetSocketLocation(ImpactSocketName);
	}
	return HoldingPawn ? HoldingPawn->GetActorLocation() + FVector::UpVector * 40.f : GetActorLocation();
}

FVector AHammerItemActor::GetAimDirection() const
{
	const AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn);
	if (!Character) return GetActorForwardVector();
	const FRotator AimRotation(Character->CurrentPitch, Character->GetActorRotation().Yaw, 0.f);
	return AimRotation.Vector();
}

void AHammerItemActor::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn || !CanUse()) return;

	LastUseTime = GetWorld()->GetTimeSeconds();

	// 모든 머신에서 휘두르는 모션 재생.
	Multicast_PlayUseMontage();

	// 휘두른 사람 기억 -> 지연 도중 소유자가 바뀌면 판정 취소.
	SwingPawn = HoldingPawn;

	if (ImpactDelay <= 0.f)
	{
		PerformImpactTrace();
		return;
	}

	GetWorldTimerManager().SetTimer(ImpactTimerHandle, this, &AHammerItemActor::PerformImpactTrace, ImpactDelay, false);
}

void AHammerItemActor::PerformImpactTrace()
{
	// 지연 도중 떨어뜨렸거나, 다른 슬롯으로 바꿨거나, 다른 사람이 주워 갔으면 타격 취소.
	if (!HoldingPawn || !bIsActiveHeld || HoldingPawn != SwingPawn.Get()) return;

	const FVector Start = GetImpactLocation();
	const FVector End = Start + GetAimDirection() * TraceDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(HoldingPawn);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius), Params))
	{
		if (ABreakableWallActor* Wall = Cast<ABreakableWallActor>(Hit.GetActor()))
		{
			Wall->ServerHit(HoldingPawn);
		}
	}
}

bool AHammerItemActor::CanUse() const
{
	return GetWorld() && (GetWorld()->GetTimeSeconds() - LastUseTime) >= UseCooldown;
}