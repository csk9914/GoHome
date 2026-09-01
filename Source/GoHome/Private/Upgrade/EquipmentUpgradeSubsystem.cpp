#include "Upgrade/EquipmentUpgradeSubsystem.h"
#include "Upgrade/EquipmentUpgradeDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Player/OxygenComponent.h"
#include "GameFramework/PlayerController.h"
#include "Upgrade/EquipmentUpgradeStateActor.h"
#include "EngineUtils.h"

bool UEquipmentUpgradeSubsystem::HasServerAuthority() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

int32 UEquipmentUpgradeSubsystem::GetUpgradeLevel(FName UpgradeId) const
{
	if (UpgradeId.IsNone())
	{
		return 1;
	}

	const FEquipmentUpgradeLevelState* State = FindLevelState(UpgradeId);
	return State ? FMath::Max(1, State->CurrentLevel) : 1;
}

FEquipmentUpgradePreview UEquipmentUpgradeSubsystem::BuildUpgradePreview(
	UEquipmentUpgradeDataAsset* UpgradeData,
	float BaseValue
) const
{
	if (!UpgradeData)
	{
		return FEquipmentUpgradePreview();
	}

	const int32 CurrentLevel = GetUpgradeLevel(UpgradeData->UpgradeId);
	return UpgradeData->BuildPreview(CurrentLevel, BaseValue);
}

EEquipmentUpgradeRequestResult UEquipmentUpgradeSubsystem::UpgradeOnce(UEquipmentUpgradeDataAsset* UpgradeData)
{
	if (!HasServerAuthority())
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	if (!UpgradeData || UpgradeData->UpgradeId.IsNone())
	{
		return EEquipmentUpgradeRequestResult::UpgradeNotFound;
	}

	const int32 CurrentLevel = GetUpgradeLevel(UpgradeData->UpgradeId);
	if (UpgradeData->IsMaxLevel(CurrentLevel))
	{
		return EEquipmentUpgradeRequestResult::AlreadyMaxLevel;
	}

	FEquipmentUpgradeLevelState* State = FindMutableLevelState(UpgradeData->UpgradeId);
	if (!State)
	{
		const int32 NewIndex = UpgradeLevels.AddDefaulted();
		State = &UpgradeLevels[NewIndex];
		State->UpgradeId = UpgradeData->UpgradeId;
		State->CurrentLevel = 1;
	}

	State->CurrentLevel = UpgradeData->GetClampedLevel(State->CurrentLevel + 1);

	OnEquipmentUpgradesChanged.Broadcast();

	return EEquipmentUpgradeRequestResult::Succeeded;
}


EEquipmentUpgradeRequestResult UEquipmentUpgradeSubsystem::RequestUpgrade(
	APlayerController* RequestingPlayer,
	UEquipmentUpgradeDataAsset* UpgradeData
)
{
	if (!HasServerAuthority())
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	if (!IsValid(RequestingPlayer))
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	if (!UpgradeData || UpgradeData->UpgradeId.IsNone())
	{
		return EEquipmentUpgradeRequestResult::UpgradeNotFound;
	}

	// 지금은 산소만 실제 적용 가능하니까 서버에서도 막기
	if (UpgradeData->EffectType != EEquipmentUpgradeEffectType::OxygenCapacity)
	{
		return EEquipmentUpgradeRequestResult::NoEffectReceiver;
	}

	AEquipmentUpgradeStateActor* StateActor = GetOrCreateStateActor();
	if (!StateActor)
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	const EEquipmentUpgradeRequestResult UpgradeResult = UpgradeOnce(UpgradeData);
	if (UpgradeResult != EEquipmentUpgradeRequestResult::Succeeded)
	{
		return UpgradeResult;
	}

	const int32 NewLevel = GetUpgradeLevel(UpgradeData->UpgradeId);
	StateActor->SetUpgradeLevel(UpgradeData->UpgradeId, NewLevel);

	UWorld* World = GetWorld();
	if (!World)
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		if (APawn* Pawn = PlayerController->GetPawn())
		{
			ApplyUpgradeToActor(UpgradeData, Pawn);
		}
	}

	return EEquipmentUpgradeRequestResult::Succeeded;
}


EEquipmentUpgradeRequestResult UEquipmentUpgradeSubsystem::ApplyUpgradeToActor(
	UEquipmentUpgradeDataAsset* UpgradeData,
	AActor* TargetActor
)
{
	// 강화 효과 적용은 서버에서만 한다.
	// 이유: 산소 최대치 같은 실제 게임플레이 수치는 서버가 권한을 가져야 한다.
	if (!HasServerAuthority())
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	// 강화 데이터가 없거나 ID가 비어 있으면 어떤 강화를 적용해야 하는지 알 수 없다.
	if (!UpgradeData || UpgradeData->UpgradeId.IsNone())
	{
		return EEquipmentUpgradeRequestResult::UpgradeNotFound;
	}

	// 적용 대상 액터가 없으면 산소 컴포넌트도 찾을 수 없다.
	if (!IsValid(TargetActor))
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	// 현재 저장된 강화 레벨을 가져온다.
	// 예: 산소 강화가 Lv2라면 2가 나온다.
	const int32 CurrentLevel = GetUpgradeLevel(UpgradeData->UpgradeId);

	// 현재 레벨의 보너스 값을 가져온다.
	// 예: Lv2 BonusValue = 3 이면 산소 +3칸.
	const float BonusValue = UpgradeData->GetBonusValueAtLevel(CurrentLevel);

	switch (UpgradeData->EffectType)
	{
	case EEquipmentUpgradeEffectType::OxygenCapacity:
	{
		// 산소 강화면 대상 액터에서 OxygenComponent를 찾는다.
		UOxygenComponent* OxygenComponent = TargetActor->FindComponentByClass<UOxygenComponent>();
		if (!OxygenComponent)
		{
			return EEquipmentUpgradeRequestResult::NoEffectReceiver;
		}

		// 산소 컴포넌트에 강화 보너스를 적용한다.
		// OxygenComponent 안에서 GetMaxOxygen() = MaxOxygen + MaxOxygenBonus로 계산된다.
		OxygenComponent->SetMaxOxygenBonus(BonusValue);

		return EEquipmentUpgradeRequestResult::Succeeded;
	}

	case EEquipmentUpgradeEffectType::CarryWeightLimit:
		// 무게 강화는 나중에 CarryWeightComponent 연결할 때 채운다.
		return EEquipmentUpgradeRequestResult::NoEffectReceiver;

	default:
		return EEquipmentUpgradeRequestResult::UpgradeNotFound;
	}
}

void UEquipmentUpgradeSubsystem::SetUpgradeLevelForSync(FName UpgradeId, int32 NewLevel)
{
	if (UpgradeId.IsNone())
	{
		return;
	}

	FEquipmentUpgradeLevelState* State = FindMutableLevelState(UpgradeId);
	if (!State)
	{
		const int32 NewIndex = UpgradeLevels.AddDefaulted();
		State = &UpgradeLevels[NewIndex];
		State->UpgradeId = UpgradeId;
	}

	State->CurrentLevel = FMath::Max(1, NewLevel);
	OnEquipmentUpgradesChanged.Broadcast();
}

FEquipmentUpgradeLevelState* UEquipmentUpgradeSubsystem::FindMutableLevelState(FName UpgradeId)
{
	return UpgradeLevels.FindByPredicate(
		[UpgradeId](const FEquipmentUpgradeLevelState& State)
		{
			return State.UpgradeId == UpgradeId;
		}
	);
}

const FEquipmentUpgradeLevelState* UEquipmentUpgradeSubsystem::FindLevelState(FName UpgradeId) const
{
	return UpgradeLevels.FindByPredicate(
		[UpgradeId](const FEquipmentUpgradeLevelState& State)
		{
			return State.UpgradeId == UpgradeId;
		}
	);
}

void UEquipmentUpgradeSubsystem::RegisterStateActor(AEquipmentUpgradeStateActor* StateActor)
{
	if (!IsValid(StateActor))
	{
		return;
	}

	if (CachedStateActor == StateActor)
	{
		return;
	}

	if (IsValid(CachedStateActor))
	{
		CachedStateActor->OnEquipmentUpgradeStateChanged.RemoveDynamic(
			this,
			&UEquipmentUpgradeSubsystem::HandleStateActorChanged
		);
	}

	CachedStateActor = StateActor;

	CachedStateActor->OnEquipmentUpgradeStateChanged.AddUniqueDynamic(
		this,
		&UEquipmentUpgradeSubsystem::HandleStateActorChanged
	);

	// 서버는 Subsystem이 원본이다.
	// 그래서 서버에서는 Subsystem의 현재 값을 StateActor에 밀어 넣는다.
	if (HasServerAuthority())
	{
		for (const FEquipmentUpgradeLevelState& LevelState : UpgradeLevels)
		{
			CachedStateActor->SetUpgradeLevel(LevelState.UpgradeId, LevelState.CurrentLevel);
		}

		return;
	}

	// 클라이언트는 StateActor가 받은 복제 값을 읽어온다.
	SyncLevelsFromStateActor();
	OnEquipmentUpgradesChanged.Broadcast();
}

AEquipmentUpgradeStateActor* UEquipmentUpgradeSubsystem::GetOrCreateStateActor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (IsValid(CachedStateActor) && CachedStateActor->GetWorld() == World)
	{
		return CachedStateActor;
	}

	CachedStateActor = nullptr;

	for (TActorIterator<AEquipmentUpgradeStateActor> It(World); It; ++It)
	{
		RegisterStateActor(*It);
		return *It;
	}

	// 클라이언트는 StateActor를 만들면 안 된다.
	// 서버가 만든 걸 복제받기만 해야 한다.
	if (World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEquipmentUpgradeStateActor* StateActor = World->SpawnActor<AEquipmentUpgradeStateActor>(
		AEquipmentUpgradeStateActor::StaticClass(),
		FTransform::Identity,
		SpawnParams
	);

	if (StateActor)
	{
		RegisterStateActor(StateActor);
	}

	return StateActor;
}

void UEquipmentUpgradeSubsystem::HandleStateActorChanged()
{
	SyncLevelsFromStateActor();
	OnEquipmentUpgradesChanged.Broadcast();
}

void UEquipmentUpgradeSubsystem::SyncLevelsFromStateActor()
{
	if (!IsValid(CachedStateActor))
	{
		return;
	}

	UpgradeLevels = CachedStateActor->GetUpgradeLevels();
}