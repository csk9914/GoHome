#include "Upgrade/EquipmentUpgradeStateActor.h"

#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Upgrade/EquipmentUpgradeSubsystem.h"

AEquipmentUpgradeStateActor::AEquipmentUpgradeStateActor()
{
	// 매 프레임 돌 필요 없다.
	// 강화 레벨은 바뀔 때만 RepNotify로 알려주면 된다.
	PrimaryActorTick.bCanEverTick = false;

	// 이 액터는 서버에서 클라이언트로 복제되어야 한다.
	bReplicates = true;

	// 강화 수치는 팀 공용 정보라 모든 클라이언트가 알아야 한다.
	bAlwaysRelevant = true;

	// 위치 이동을 복제할 필요 없다.
	// 이 액터는 눈에 보이는 설치물이 아니라, 네트워크 상태 저장판 역할이다.
	SetReplicatingMovement(false);
}

void AEquipmentUpgradeStateActor::BeginPlay()
{
	Super::BeginPlay();

	// 월드에 생성되면 Subsystem에게 "나 여기 있음" 하고 등록한다.
	// Subsystem은 이 액터를 통해 서버 강화 레벨을 클라이언트 UI까지 동기화한다.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEquipmentUpgradeSubsystem* Subsystem = GameInstance->GetSubsystem<UEquipmentUpgradeSubsystem>())
		{
			Subsystem->RegisterStateActor(this);
		}
	}
}

int32 AEquipmentUpgradeStateActor::GetUpgradeLevel(FName UpgradeId) const
{
	if (UpgradeId.IsNone())
	{
		return 1;
	}

	const FEquipmentUpgradeLevelState* State = UpgradeLevels.FindByPredicate(
		[UpgradeId](const FEquipmentUpgradeLevelState& LevelState)
		{
			return LevelState.UpgradeId == UpgradeId;
		}
	);

	return State ? FMath::Max(1, State->CurrentLevel) : 1;
}

bool AEquipmentUpgradeStateActor::SetUpgradeLevel(FName UpgradeId, int32 NewLevel)
{
	// 강화 레벨 변경은 서버만 할 수 있다.
	if (!HasAuthority())
	{
		return false;
	}

	if (UpgradeId.IsNone())
	{
		return false;
	}

	const int32 ClampedNewLevel = FMath::Max(1, NewLevel);

	FEquipmentUpgradeLevelState* State = UpgradeLevels.FindByPredicate(
		[UpgradeId](const FEquipmentUpgradeLevelState& LevelState)
		{
			return LevelState.UpgradeId == UpgradeId;
		}
	);

	if (!State)
	{
		const int32 NewIndex = UpgradeLevels.AddDefaulted();
		State = &UpgradeLevels[NewIndex];
		State->UpgradeId = UpgradeId;
	}

	if (State->CurrentLevel == ClampedNewLevel)
	{
		return false;
	}

	State->CurrentLevel = ClampedNewLevel;

	// 서버 쪽 UI, 특히 listen server/host도 바로 갱신되게 알려준다.
	OnEquipmentUpgradeStateChanged.Broadcast();

	// 클라이언트에게 가능한 빨리 복제하라고 요청한다.
	ForceNetUpdate();

	return true;
}

void AEquipmentUpgradeStateActor::OnRep_UpgradeLevels()
{
	// 클라이언트가 새 강화 레벨을 받으면 UI 갱신 신호를 보낸다.
	OnEquipmentUpgradeStateChanged.Broadcast();
}

void AEquipmentUpgradeStateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// UpgradeLevels 배열을 서버에서 클라이언트로 복제한다.
	DOREPLIFETIME(AEquipmentUpgradeStateActor, UpgradeLevels);
}