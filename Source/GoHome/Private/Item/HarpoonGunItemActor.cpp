
#include "Item/HarpoonGunItemActor.h"
#include "Item/HarpoonCosmeticActor.h"
#include "Item/ItemActorBase.h"
#include "Player/GoHomeCharacter.h"
#include "Net/UnrealNetwork.h"


void AHarpoonGunItemActor::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn || !CanUse()) return;

	AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn);
	if (!Character) return;

	LastUseTime = GetWorld()->GetTimeSeconds();

	// 판정은 카메라(화면 중앙 = 크로스헤어) 기준. 총구 기준이면 화면 중앙과 어긋난 방향으로 나가서
	// 크로스헤어가 가리키는 곳과 실제 판정 지점이 안 맞는 문제가 있었음(확인됨).
	const FRotator AimRotation(Character->CurrentPitch, Character->GetActorRotation().Yaw, 0.f);
	const FVector Start = Character->GetCameraWorldLocation();
	const FVector End = Start + AimRotation.Vector() * TraceDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(HoldingPawn);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius),
		Params);

	AItemActorBase* Target = bHit ? Cast<AItemActorBase>(Hit.GetActor()) : nullptr;

	// 연출(로프/훅 비주얼)은 총구 위치에서 시작 - 판정용 트레이스와는 분리된 순수 코스메틱.
	FVector MuzzleLocation = Start;
	if (MuzzleSocketName != NAME_None && MeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);
	}

	// CoopCarryObject는 타입이 안 맞아 Cast 실패로 자동 제외.
	// 이미 확보된(bIsBeingClaimed) 아이템도 CanInteract()에서 자동 제외.
	if (!Target || !Target->CanInteract(HoldingPawn))
	{
		FireEventStart = MuzzleLocation;
		FireEventEnd = bHit ? Hit.Location : End; // 벽/바닥에 막히면 그 지점까지만, 아무것도 없으면 사거리 끝까지.
		++FireEventId;
		PlayFireCosmetic(); // 서버 자신은 RepNotify가 안 뜨므로 수동 호출.
		return;
	}

	Target->SetBeingClaimed(true);
	Target->SetExternallyPositioned(true);
	RetrievingTarget = Target;
	RetrieveElapsed = 0.f;
	bReturnStartCaptured = false;

	// 회수 도중 자기 자신(플레이어)의 콜리전에 막혀 튕기는 것 방지 - 벽/바닥은 그대로 막힘.
	RetrievingTarget->MeshComponent->IgnoreActorWhenMoving(HoldingPawn, true);

	FireEventStart = MuzzleLocation;
	FireEventEnd = Hit.Location;
	++FireEventId;
	PlayFireCosmetic();

	SetActorTickEnabled(true);
}

bool AHarpoonGunItemActor::CanUse() const
{
	if (RetrievingTarget) return false; // 이미 회수 진행 중.
	return GetWorld() && (GetWorld()->GetTimeSeconds() - LastUseTime) >= UseCooldown;
}

void AHarpoonGunItemActor::Tick(float DeltaTime)
{
	if (RetrievingTarget)
	{
		// 회수 진행 중엔 부모(AItemActorBase)의 부력 Tick 대신 이 로직으로 완전히 대체한다.
		// 이 아이템은 회수 중 항상 손에 들려 물리가 꺼진 상태라 Super::Tick()을 불러도 의미가 없을 뿐 아니라,
		// Super::Tick()이 "물리 시뮬레이션 중이 아니면 SetActorTickEnabled(false)"로 방금 우리가 켠
		// Tick을 도로 꺼버리는 충돌이 생긴다 - 그래서 회수 중엔 Super 호출 자체를 건너뛴다.
		if (!HasAuthority())
		{
			return;
		}

		if (!IsValid(RetrievingTarget) || !HoldingPawn || !IsValid(HoldingPawn))
		{
			AbortRetrieve();
			return;
		}

		RetrieveElapsed += DeltaTime;

		if (RetrieveElapsed < OutboundDuration)
		{
			return; // 왕복(발사) 단계 - 연출만 재생 중, 대상은 그대로 둠.
		}

		if (!bReturnStartCaptured)
		{
			ReturnStartLocation = RetrievingTarget->GetActorLocation();
			bReturnStartCaptured = true;
		}

		const float ReturnAlpha = FMath::Clamp((RetrieveElapsed - OutboundDuration) / ReturnDuration, 0.f, 1.f);

		// 목적지를 매 틱 "플레이어 현재 위치" 기준으로 재계산 -> 회수 도중 이동해도 자연스럽게 따라감.
		const FVector DropOffPoint = HoldingPawn->GetActorLocation()
			+ HoldingPawn->GetActorForwardVector() * DropOffDistance;

		const FVector NewPos = FMath::Lerp(ReturnStartLocation, DropOffPoint, ReturnAlpha);
		RetrievingTarget->SetActorLocation(NewPos, true); // sweep=true -> 장애물에 자연스럽게 막힘.

		if (ReturnAlpha >= 1.f)
		{
			RetrievingTarget->MeshComponent->IgnoreActorWhenMoving(HoldingPawn, false); // 무시 해제.
			RetrievingTarget->SetBeingClaimed(false);
			RetrievingTarget->SetExternallyPositioned(false);
			RetrievingTarget = nullptr;
			bReturnStartCaptured = false;
			SetActorTickEnabled(false);
		}

		return;
	}

	Super::Tick(DeltaTime); // 평소(회수 중 아님)엔 부모의 부력 로직 그대로 사용.
}

void AHarpoonGunItemActor::AbortRetrieve()
{
	if(RetrievingTarget && IsValid(RetrievingTarget))
	{ 
		RetrievingTarget->MeshComponent->IgnoreActorWhenMoving(HoldingPawn, false);
		RetrievingTarget->SetBeingClaimed(false);
		RetrievingTarget->SetExternallyPositioned(false);
	}
	RetrievingTarget = nullptr;
	bReturnStartCaptured = false;
	SetActorTickEnabled(false);
}

void AHarpoonGunItemActor::ServerDrop()
{
	// HoldingPawn이 nullptr 되기 전에 먼저 회수 중단 처리.
	// -> AbortRetrieve가 진짜 폰 참조로 IgnoreActorWhenMoving를 정확히 해제할 수 있게 함.
	if (RetrievingTarget)
	{
		AbortRetrieve();
	}

	Super::ServerDrop();
}

void AHarpoonGunItemActor::OnRep_FireEventId()
{
	PlayFireCosmetic();
}


void AHarpoonGunItemActor::PlayFireCosmetic()
{
	if (!GetWorld() || !CosmeticClass) return;

	if (AHarpoonCosmeticActor* Cosmetic = GetWorld()->SpawnActor<AHarpoonCosmeticActor>(CosmeticClass, FireEventStart, FRotator::ZeroRotator))
	{
		Cosmetic->Play(FireEventStart, FireEventEnd, OutboundDuration, ReturnDuration, MeshComponent, MuzzleSocketName);
	}
}

void AHarpoonGunItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHarpoonGunItemActor, FireEventStart);
	DOREPLIFETIME(AHarpoonGunItemActor, FireEventEnd);
	DOREPLIFETIME(AHarpoonGunItemActor, FireEventId);
}