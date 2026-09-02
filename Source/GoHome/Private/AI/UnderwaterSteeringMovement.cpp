// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/UnderwaterSteeringMovement.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UUnderwaterSteeringMovement::UUnderwaterSteeringMovement()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UUnderwaterSteeringMovement::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}
FVector UUnderwaterSteeringMovement::GetTraceStartLocation() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	// BP에서 직접 지정한 컴포넌트가 있으면 그 위치에서 Trace를 시작한다.
	if (TraceStartComponent)
	{
		return TraceStartComponent->GetComponentLocation();
	}

	// 지정 컴포넌트가 없으면 이름으로 EyePoint를 찾아서 Trace 시작점으로 쓴다.
	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == EyePointComponentName)
		{
			return SceneComponent->GetComponentLocation();
		}

	}

	// EyePoint도 못 찾으면 Actor 위치를 마지막 기본값으로 사용한다.
	return Owner->GetActorLocation();
}




void UUnderwaterSteeringMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UUnderwaterSteeringMovement::Steering_MoveToLocation(FVector TargetLocation, float MoveSpeed, float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector ToTarget = TargetLocation - OwnerLocation;

	// 목표 지점에 충분히 가까우면 이동/회피 상태를 멈춘다.
	if (ToTarget.Size() <= ArriveDistance)
	{
		Steering_Stop();
		return;
	}

	// 포지션으로 가야 하는 순수 목표 방향이다.
	const FVector DesiredDir = ToTarget.GetSafeNormal();
	if (DesiredDir.IsNearlyZero())
	{
		return;
	}

	// 목표 방향에 벽 회피를 섞어서 최종 이동 방향을 만든다.
	const FVector SteeringDir = GetSteeringDirection(DesiredDir, DeltaTime);

	SmoothMoveAndRotate(SteeringDir, MoveSpeed, DeltaTime);
}

void UUnderwaterSteeringMovement::SmoothMoveAndRotate(FVector MoveDir, float MoveSpeed, float DeltaTime)
{
	AActor* Owner = GetOwner();
	APawn* PawnOwner = Cast<APawn>(Owner);

	if (!Owner || !PawnOwner)
	{
		return;
	}

	const FVector Dir = MoveDir.GetSafeNormal();

	if (Dir.IsNearlyZero())
	{
		return;
	}

	CurrentSteeringDir = Dir;

	// 방향을 바로 바꾸지 않고 보간해서 헤엄치는 느낌을 만든다.
	SmoothedMoveDir = FMath::VInterpTo(
		SmoothedMoveDir,
		Dir,
		DeltaTime,
		SwimDirectionInterpSpeed
	).GetSafeNormal();

	if (SmoothedMoveDir.IsNearlyZero())
	{
		return;
	}

	if (UFloatingPawnMovement* FloatingMovement = GetFloatingMovement())
	{
		FloatingMovement->MaxSpeed = MoveSpeed;
	}

	// 실제 이동은 FloatingPawnMovement가 처리한다.
	PawnOwner->AddMovementInput(SmoothedMoveDir, MoveInputStrength);

	const FRotator TargetRotation = SmoothedMoveDir.Rotation();

	// 몸 회전도 보간해서 급격하게 꺾이지 않게 한다.
	const FRotator NewRotation = FMath::RInterpTo(
		Owner->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		TurnSpeed
	);

	Owner->SetActorRotation(NewRotation);
}

FVector UUnderwaterSteeringMovement::GetSteeringDirection(FVector DesiredDir, float DeltaTime)
{
	const FVector Desired = DesiredDir.GetSafeNormal();

	if (Desired.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	AActor* Owner = GetOwner();
	// 이동하려는 목표 방향을 기준으로 회피 방향을 만든다. Actor Forward를 쓰면 몸이 아직 덜 돌아간 순간에 엉뚱한 방향을 검사할 수 있다.
	const FVector ForwardDir = Desired;
	FVector RightDir = FVector::CrossProduct(FVector::UpVector, ForwardDir).GetSafeNormal();
	if (RightDir.IsNearlyZero())
	{
		// 목표 방향이 거의 수직이면 UpVector와 외적한 RightDir이 0에 가까워지므로 Owner의 현재 오른쪽 방향을 예외값으로 쓴다.
		RightDir = Owner ? Owner->GetActorRightVector().GetSafeNormal() : FVector::RightVector;
	}
	const FVector LeftDir = -RightDir;
	const FVector UpDir = FVector::UpVector;
	const FVector DownDir = -FVector::UpVector;
	const FVector BackDir = -ForwardDir;

	// 뒤로 빠지는 중이어도 목표 방향이 다시 열리면 즉시 탈출 상태를 끊는다.
	if (bForceEscape && !IsDirectionBlocked(ForwardDir))
	{
		bForceEscape = false;
		ForceEscapeTime = 0.0f;
		ForceEscapeDir = FVector::ZeroVector;
		bAvoidLocked = false;
		AvoidLockTime = 0.0f;
		AvoidLockedDir = FVector::ZeroVector;
	}

	// 강제 탈출 중이면 다른 계산보다 우선해서 뒤로 빠진다.
	FVector OutDir;
	if (UpdateForceEscape(DeltaTime, OutDir))
	{
		return OutDir;
	}

	// 한번 고른 회피 방향은 잠깐 유지해서 좌우로 떨리는 현상을 줄인다.
	if (bAvoidLocked)
	{
		AvoidLockTime -= DeltaTime;

		if (AvoidLockTime > 0.0f && !AvoidLockedDir.IsNearlyZero())
		{
			return AvoidLockedDir.GetSafeNormal();
		}

		bAvoidLocked = false;
		AvoidLockTime = 0.0f;
		AvoidLockedDir = FVector::ZeroVector;
	}

	// 벽검사는 목표 지점 방향 기준 앞/오른쪽/왼쪽/아래/위 5방향으로 한다.
	const bool bFrontBlocked = IsDirectionBlocked(ForwardDir);
	const bool bRightBlocked = IsDirectionBlocked(RightDir);
	const bool bLeftBlocked = IsDirectionBlocked(LeftDir);
	const bool bDownBlocked = IsDirectionBlocked(DownDir);
	const bool bUpBlocked = IsDirectionBlocked(UpDir);

	// 앞이 안 막혔으면 회피하지 않고 포지션 방향으로 이동한다.
	if (!bFrontBlocked)
	{
		return Desired;
	}

	CandidateDirs.Reset();

	// 앞이 막혔을 때만 비어 있는 방향을 후보로 모은다.
	if (!bRightBlocked)
	{
		CandidateDirs.Add(RightDir);
	}

	if (!bLeftBlocked)
	{
		CandidateDirs.Add(LeftDir);
	}

	if (!bDownBlocked)
	{
		CandidateDirs.Add(DownDir);
	}

	if (!bUpBlocked)
	{
		CandidateDirs.Add(UpDir);
	}

	// 후보가 있으면 랜덤 후보, 모두 막혔으면 뒤로 빠진다.
	return PickCandidateOrBack(BackDir);
}

bool UUnderwaterSteeringMovement::IsDirectionBlocked(FVector CheckDir) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return false;
	}

	const FVector Dir = CheckDir.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return false;
	}
	const FVector Start = GetTraceStartLocation();
	const FVector End = Start + Dir * ObstacleDetectDistance;

	FHitResult Hit;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner);

	// Owner 자신은 무시하고, 지정한 Trace 채널에 Block 되는 벽만 감지한다.
	const bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		Owner,
		Start,
		End,
		ObstacleTraceRadius,
		UEngineTypes::ConvertToTraceType(ObstacleTraceChannel),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		Hit,
		true
	);

	return bHit;
	
}

bool UUnderwaterSteeringMovement::UpdateForceEscape(float DeltaTime, FVector& OutDir)
{
	if (!bForceEscape)
	{
		OutDir = FVector::ZeroVector;
		return false;
	}

	ForceEscapeTime -= DeltaTime;

	if (ForceEscapeTime > 0.0f && !ForceEscapeDir.IsNearlyZero())
	{
		OutDir = ForceEscapeDir.GetSafeNormal();
		return true;
	}

	bForceEscape = false;
	ForceEscapeTime = 0.0f;
	ForceEscapeDir = FVector::ZeroVector;

	OutDir = FVector::ZeroVector;
	return false;
}

void UUnderwaterSteeringMovement::AddCandidateIfNotBlocked(FVector CheckDir)
{
	const FVector Dir = CheckDir.GetSafeNormal();

	if (Dir.IsNearlyZero())
	{
		return;
	}

	if (!IsDirectionBlocked(Dir))
	{
		CandidateDirs.Add(Dir);
	}
}

FVector UUnderwaterSteeringMovement::PickCandidateOrBack(FVector BackDir)
{
	if (CandidateDirs.Num() > 0)
	{
		// 한 방향만 계속 고르면 빙글빙글 돌 수 있어서 후보 중 하나를 랜덤으로 고른다.
		const int32 RandomIndex = FMath::RandRange(0, CandidateDirs.Num() - 1);
		AvoidLockedDir = CandidateDirs[RandomIndex].GetSafeNormal();
		bAvoidLocked = true;
		AvoidLockTime = AvoidLockDuration;

		return AvoidLockedDir;
	}

	const FVector SafeBackDir = BackDir.GetSafeNormal();

	// 후보가 하나도 없으면 막힌 공간이라고 보고 뒤로 빠지는 방향을 잠깐 강제한다.
	AvoidLockedDir = SafeBackDir;
	bAvoidLocked = true;
	AvoidLockTime = AvoidLockDuration;

	ForceEscapeDir = SafeBackDir;
	bForceEscape = true;
	ForceEscapeTime = ForceEscapeDuration;

	return SafeBackDir;
}

bool UUnderwaterSteeringMovement::Steering_HasArrived(FVector TargetLocation) const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return false;
	}

	return FVector::Dist(Owner->GetActorLocation(), TargetLocation) <= ArriveDistance;
}

void UUnderwaterSteeringMovement::Steering_Stop()
{
	CurrentSteeringDir = FVector::ZeroVector;
	SmoothedMoveDir = FVector::ZeroVector;

	bAvoidLocked = false;
	AvoidLockTime = 0.0f;
	AvoidLockedDir = FVector::ZeroVector;

	bForceEscape = false;
	ForceEscapeTime = 0.0f;
	ForceEscapeDir = FVector::ZeroVector;
}

void UUnderwaterSteeringMovement::SetTraceStartComponent(USceneComponent* NewTraceStartComponent)
{
	TraceStartComponent = NewTraceStartComponent;
}

UFloatingPawnMovement* UUnderwaterSteeringMovement::GetFloatingMovement() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return Owner->FindComponentByClass<UFloatingPawnMovement>();
}
