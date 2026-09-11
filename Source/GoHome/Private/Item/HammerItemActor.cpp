#include "Item/HammerItemActor.h"
#include "Interaction/BreakableWallActor.h"
#include "Player/GoHomeCharacter.h"

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
	// 코스메틱(휘두르는 모션/타격음)이 필요하면 여기 추가.
}

bool AHammerItemActor::CanUse() const
{
	return GetWorld() && (GetWorld()->GetTimeSeconds() - LastUseTime) >= UseCooldown;
}