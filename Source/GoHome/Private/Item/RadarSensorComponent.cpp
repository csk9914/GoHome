


#include "Item/RadarSensorComponent.h"
#include "Item/ItemActorBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

URadarSensorComponent::URadarSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// 순수 로컬 연출이라 복제하지 않음
	SetIsReplicatedByDefault(false);
}

void URadarSensorComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsOperational())
	{
		ResetCycle();
		return;
	}

	// 꺼져 있다 켜졌으면 항상 새 사이클(0°)에서 시작
	if (!bCycleActive)
	{
		bCycleActive = true;
		CycleTime = 0.f;
		LastSweepAngle = 0.f;
		BeginSweep();
	}

	const float SafeSweep = FMath::Max(SweepDuration, 0.01f);
	const float SafeFade = FMath::Max(FadeDuration, 0.f);
	const float CycleDuration = SafeSweep + SafeFade;

	CycleTime += DeltaTime;

	// 사이클이 끝났으면 백지에서 새 스윕을 시작
	// 막대가 매 사이클 0°에서 다시 출발하므로 360->0 이음매를 넘는 경우가 아예 없음
	if (CycleTime >= CycleDuration)
	{
		CycleTime = FMath::Fmod(CycleTime, CycleDuration);
		LastSweepAngle = 0.f;
		BeginSweep();
	}

	// 이번 프레임의 막대 각도. 
	float CurrentSweepAngle = 360.f;
	bool bSweeping = false;

	if (CycleTime < SafeSweep)
	{
		bSweeping = true;
		CurrentSweepAngle = (CycleTime / SafeSweep) * 360.f;
	}

	// 지난 프레임 각도부터 지금 각도까지의 "구간"을 검사
	CapturePings(LastSweepAngle, CurrentSweepAngle);
	LastSweepAngle = CurrentSweepAngle;

	// 페이드 - 스윕이 끝나는 순간부터 모든 핑이 다 같이 흐려짐
	float PingAlpha = 1.f;
	float RechargeProgress = 0.f;

	if (!bSweeping && SafeFade > 0.f)
	{
		RechargeProgress = FMath::Clamp((CycleTime - SafeSweep) / SafeFade, 0.f, 1.f);
		PingAlpha = 1.f - RechargeProgress;
	}

	FRadarSnapshot Snapshot;
	Snapshot.bActive = true;
	Snapshot.Pings = Pings;
	Snapshot.SweepAngle = CurrentSweepAngle;
	Snapshot.bSweeping = bSweeping;
	Snapshot.PingAlpha = PingAlpha;
	Snapshot.RechargeProgress = RechargeProgress;
	Snapshot.ViewYaw = GetViewYaw();

	OnRadarUpdated.Broadcast(Snapshot);
}

void URadarSensorComponent::ResetCycle()
{
	if (!bCycleActive)
	{
		return;
	}

	bCycleActive = false;
	CycleTime = 0.f;
	LastSweepAngle = 0.f;
	Pings.Reset();
	PendingMonsters.Reset();

	// 꺼졌다는 사실을 위젯에 딱 한 번 알림
	FRadarSnapshot Off;
	OnRadarUpdated.Broadcast(Off);
}

void URadarSensorComponent::BeginSweep()
{
	Pings.Reset();
	PendingMonsters.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const TSubclassOf<AActor>& DetectClass : DetectableClasses)
	{
		if (!DetectClass)
		{
			continue;
		}

		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, DetectClass, Found);

		for (AActor* Monster : Found)
		{
			if (!Monster)
			{
				continue;
			}

			PendingMonsters.AddUnique(TWeakObjectPtr<AActor>(Monster));
		}
	}
}

void URadarSensorComponent::CapturePings(float FromAngle, float ToAngle)
{
	if (ToAngle <= FromAngle || DetectRadius <= 0.f)
	{
		return;
	}

	const APawn* Operator = GetOperatorPawn();
	if (!Operator)
	{
		return;
	}

	const FVector Origin = Operator->GetActorLocation();
	const float RadiusSq = DetectRadius * DetectRadius;

	// 핑을 찍은 몬스터는 목록에서 빼기 때문에 뒤에서부터 순회
	for (int32 Index = PendingMonsters.Num() - 1; Index >= 0; --Index)
	{
		AActor* Monster = PendingMonsters[Index].Get();

		// 사라진 액터는 정리만 하고 넘어감
		if (!Monster)
		{
			PendingMonsters.RemoveAtSwap(Index);
			continue;
		}

		const FVector Offset = Monster->GetActorLocation() - Origin;

		// 수직으로 너무 벗어났으면 제외. 목록엔 남겨둠
		if (FMath::Abs(Offset.Z) > DetectHeightRange)
		{
			continue;
		}

		// 반경 판정은 수평 거리로만. 깊이는 화살표로 따로 보여줌
		const FVector2D Horizontal(Offset.X, Offset.Y);
		if (Horizontal.SizeSquared() > RadiusSq)
		{
			continue;
		}

		// 월드 X=북, Y=동 기준의 나침반 각도. Atan2(Y, X)가 그대로 북=0°, 동=90°가 된다.
		const float MonsterAngle = FRotator::ClampAxis(
			FMath::RadiansToDegrees(FMath::Atan2(Horizontal.Y, Horizontal.X)));

		// 막대가 이번 프레임에 지나온 구간 안에 있을 때만 찍음
		if (MonsterAngle < FromAngle || MonsterAngle >= ToAngle)
		{
			continue;
		}

		FRadarPing Ping;

		// 월드(X=북, Y=동) -> UMG 화면(X=오른쪽, Y=아래).
		// 북쪽이 화면 위로 가야 하므로 X에 음수를 씌워 Y축으로 보냄
		Ping.NormalizedPos = FVector2D(Horizontal.Y, -Horizontal.X) / DetectRadius;
		Ping.HeightDelta = Offset.Z;

		if (Offset.Z > DepthArrowThreshold)
		{
			Ping.Depth = ERadarDepth::Above;
		}
		else if (Offset.Z < -DepthArrowThreshold)
		{
			Ping.Depth = ERadarDepth::Below;
		}
		else
		{
			Ping.Depth = ERadarDepth::Same;
		}

		Pings.Add(Ping);

		// 이번 스윕에서 이미 잡았다 -> 몬스터 하나당 핑 하나가 보장
		PendingMonsters.RemoveAtSwap(Index);
	}
}

APawn* URadarSensorComponent::GetOperatorPawn() const
{
	AActor* MyOwner = GetOwner();
	if (!MyOwner)
	{
		return nullptr;
	}

	// 손에 들려 있으면 부착 부모가 곧 소지자
	if (APawn* Holder = Cast<APawn>(MyOwner->GetAttachParentActor()))
	{
		return Holder;
	}

	// 폰에 직접 붙인 경우(헬멧/잠수정 등)
	return Cast<APawn>(MyOwner);
}

bool URadarSensorComponent::IsOperational() const
{
	const APawn* Operator = GetOperatorPawn();

	// 레이더 화면은 들고 있는 본인에게만 의미가 있음
	if (!Operator || !Operator->IsLocallyControlled())
	{
		return false;
	}

	// 아이템에 붙어 있으면 활성 슬롯(손에 나와 있음)일 때만 작동
	if (const AItemActorBase* OwningItem = Cast<AItemActorBase>(GetOwner()))
	{
		return OwningItem->bIsActiveHeld;
	}

	return true;
}

float URadarSensorComponent::GetViewYaw() const
{
	const APawn* Operator = GetOperatorPawn();
	if (!Operator)
	{
		return 0.f;
	}

	// 1인칭이라 캐릭터 회전과 시선이 다를 수 있다. 컨트롤 회전이 실제 보는 방향
	if (const AController* OperatorController = Operator->GetController())
	{
		return FRotator::ClampAxis(OperatorController->GetControlRotation().Yaw);
	}

	return FRotator::ClampAxis(Operator->GetActorRotation().Yaw);
}