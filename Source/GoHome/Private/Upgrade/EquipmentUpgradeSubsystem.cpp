#include "Upgrade/EquipmentUpgradeSubsystem.h"
#include "Upgrade/EquipmentUpgradeDataAsset.h"
#include "Upgrade/EquipmentUpgradeStateActor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/OxygenComponent.h"
#include "Player/CarryWeightComponent.h"
#include "Save/GoHomeSaveSubsystem.h"
#include "EngineUtils.h"
#include "UObject/UObjectGlobals.h"

void UEquipmentUpgradeSubsystem::Initialize(
	FSubsystemCollectionBase& CollectionBase)
{
	Super::Initialize(CollectionBase);

	// SaveSubsystem이 먼저 세이브 파일을 읽도록 보장한다.
	CollectionBase.InitializeDependency<UGoHomeSaveSubsystem>();

	UGoHomeSaveSubsystem* SaveSubsystem =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UGoHomeSaveSubsystem>()
		: nullptr;

	if (!SaveSubsystem)
	{
		return;
	}

	// SaveGame에 저장된 강화 레벨을 런타임 배열로 가져온다.
	UpgradeLevels = SaveSubsystem->GetSavedUpgradeLevels();

	// 맵이 바뀔 때마다 새 Pawn을 감시한다.
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UEquipmentUpgradeSubsystem::HandlePostLoadMap
	);

	// 이미 월드가 존재한다면 즉시 감시를 시작한다.
	if (UWorld* World = GetWorld())
	{
		BindToWorld(World);
	}
}

void UEquipmentUpgradeSubsystem::Deinitialize()
{
	UnbindFromWorld();

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	Super::Deinitialize();
}

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

bool UEquipmentUpgradeSubsystem::IsUpgradeUnlocked(UEquipmentUpgradeDataAsset* UpgradeData) const
{
	// DA가 없거나 ID가 비어 있으면 안전하게 잠금 처리.
	if (!UpgradeData || UpgradeData->UpgradeId.IsNone())
	{
		return false;
	}

	const FEquipmentUpgradeUnlockRequirement& Requirement =
		UpgradeData->UnlockRequirement;

	// RequiredUpgradeId가 None이면 선행 조건 없음 = 처음부터 해금.
	if (Requirement.RequiredUpgradeId.IsNone())
	{
		return true;
	}

	const int32 RequiredCurrentLevel =
		GetUpgradeLevel(Requirement.RequiredUpgradeId);

	return RequiredCurrentLevel >= Requirement.RequiredLevel;
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

	if (!IsUpgradeUnlocked(UpgradeData))
	{
		return EEquipmentUpgradeRequestResult::UpgradeLocked;
	}

	const int32 CurrentLevel = GetUpgradeLevel(UpgradeData->UpgradeId);
	if (UpgradeData->IsMaxLevel(CurrentLevel))
	{
		return EEquipmentUpgradeRequestResult::AlreadyMaxLevel;
	}

	// 세이브 시스템을 가져온다.
	UGameInstance* GameInstance = GetGameInstance();

	UGoHomeSaveSubsystem* SaveSubsystem =
		GameInstance
		? GameInstance->GetSubsystem<UGoHomeSaveSubsystem>()
		: nullptr;

	if (!SaveSubsystem)
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	// 현재 레벨에서 다음 레벨로 올라가는 비용을 계산한다.
	const int32 UpgradeCost =
		UpgradeData->GetCostToUpgradeFromLevel(CurrentLevel);

	// 현재 보유 코인을 가져온다.
	const int32 CurrentFunds =
		SaveSubsystem->GetCurrentFunds();

	// 업그레이드는 보유 코인이 비용보다 적으면 실패한다.
	// 다른 시스템에서는 음수 코인을 허용할 수 있지만,
	// 업그레이드에서는 빚을 지고 강화할 수 없게 한다.
	if (UpgradeCost < 0 || CurrentFunds < UpgradeCost)
	{
		return EEquipmentUpgradeRequestResult::NotEnoughCurrency;
	}

	// 충분한 코인이 확인된 뒤에만 실제로 차감한다.
	if (!SaveSubsystem->TrySpendFunds(UpgradeCost))
	{
		return EEquipmentUpgradeRequestResult::NotEnoughCurrency;
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

	// 런타임 강화 레벨을 SaveGame 데이터에도 복사한다.
	SaveSubsystem->SetSavedUpgradeLevels(UpgradeLevels);

	// 강화 성공 후 코인과 강화 레벨을 디스크에 저장한다.
	SaveSubsystem->SaveToDisk();

	OnEquipmentUpgradesChanged.Broadcast();

	return EEquipmentUpgradeRequestResult::Succeeded;
}


EEquipmentUpgradeRequestResult UEquipmentUpgradeSubsystem::RequestUpgrade(
	APlayerController* RequestingPlayer,
	UEquipmentUpgradeDataAsset* UpgradeData,
	int32& OutCurrentFunds
)
{
	OutCurrentFunds = 0;

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

	// 무게 업그레이드 부분
	if (UpgradeData->EffectType != EEquipmentUpgradeEffectType::OxygenCapacity
		&& UpgradeData->EffectType != EEquipmentUpgradeEffectType::CarryWeightLimit)
	{
		return EEquipmentUpgradeRequestResult::NoEffectReceiver;
	}

	// 실제 플레이어 액터들이 강화 효과를 받을 수 있는지 먼저 확인한다.
	// 이 검사를 통과한 뒤에만 코인을 차감해야 한다.
	UWorld* World = GetWorld();

	if (!World)
	{
		return EEquipmentUpgradeRequestResult::InvalidRequester;
	}

	bool bFoundPawn = false;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();

		if (!PlayerController)
		{
			continue;
		}

		APawn* Pawn = PlayerController->GetPawn();

		if (!Pawn)
		{
			continue;
		}

		bFoundPawn = true;

		if (UpgradeData->EffectType == EEquipmentUpgradeEffectType::OxygenCapacity)
		{
			if (!Pawn->FindComponentByClass<UOxygenComponent>())
			{
				return EEquipmentUpgradeRequestResult::NoEffectReceiver;
			}
		}

		if (UpgradeData->EffectType == EEquipmentUpgradeEffectType::CarryWeightLimit)
		{
			if (!Pawn->FindComponentByClass<UCarryWeightComponent>())
			{
				return EEquipmentUpgradeRequestResult::NoEffectReceiver;
			}
		}
	}

	if (!bFoundPawn)
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

	// 강화 성공 후 최신 코인을 결과값으로 돌려준다.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGoHomeSaveSubsystem* SaveSubsystem =
			GameInstance->GetSubsystem<UGoHomeSaveSubsystem>())
		{
			OutCurrentFunds = SaveSubsystem->GetCurrentFunds();
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
	{
		UCarryWeightComponent* CarryWeightComponent =
			TargetActor->FindComponentByClass<UCarryWeightComponent>();

		if (!CarryWeightComponent)
		{
			return EEquipmentUpgradeRequestResult::NoEffectReceiver;
		}

		CarryWeightComponent->SetMaxCarryWeightBonus(BonusValue);

		return EEquipmentUpgradeRequestResult::Succeeded;
	}

	default:
		return EEquipmentUpgradeRequestResult::UpgradeNotFound;
	}
}

void UEquipmentUpgradeSubsystem::ApplySavedUpgradesToActor(AActor* TargetActor)
{
	// 실제 게임플레이 수치는 서버만 변경한다.
	if (!HasServerAuthority() || !IsValid(TargetActor))
	{
		return;
	}

	// 산소 강화 데이터 에셋을 가져온다.
	UEquipmentUpgradeDataAsset* OxygenUpgrade =
		LoadObject<UEquipmentUpgradeDataAsset>(
			nullptr,
			TEXT("/Game/GoHome/Developers/LSA/Blueprint/DA_OxygenUpgrade.DA_OxygenUpgrade")
		);

	if (OxygenUpgrade)
	{
		ApplyUpgradeToActor(OxygenUpgrade, TargetActor);
	}

	// 무게 강화 데이터 에셋을 가져온다.
	UEquipmentUpgradeDataAsset* CarryWeightUpgrade =
		LoadObject<UEquipmentUpgradeDataAsset>(
			nullptr,
			TEXT("/Game/GoHome/Developers/LSA/Blueprint/DA_CarryWeightUpgrade.DA_CarryWeightUpgrade")
		);

	if (CarryWeightUpgrade)
	{
		ApplyUpgradeToActor(CarryWeightUpgrade, TargetActor);
	}
}

void UEquipmentUpgradeSubsystem::ResetUpgradeLevels()
{
	// 강화 상태 초기화는 서버에서만 한다.
	if (!HasServerAuthority())
	{
		return;
	}

	// UpgradeSubsystem의 런타임 강화 레벨을 초기화한다.
	UpgradeLevels.Reset();

	if (IsValid(CachedStateActor))
	{
		// 네트워크 상태 액터도 함께 초기화한다.
		// 이 함수가 클라이언트 복제와 UI 알림을 처리한다.
		CachedStateActor->ResetUpgradeLevels();
	}
	else
	{
		// StateActor가 없는 경우에도 서버 UI에는 변경을 알린다.
		OnEquipmentUpgradesChanged.Broadcast();
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
		// SetUpgradeLevel()이 이벤트를 발생시키면서
		// 원본 UpgradeLevels를 변경할 수 있으므로 복사본을 사용한다.
		const TArray<FEquipmentUpgradeLevelState> LevelsToSync = UpgradeLevels;

		for (const FEquipmentUpgradeLevelState& LevelState : LevelsToSync)
		{
			CachedStateActor->SetUpgradeLevel(
				LevelState.UpgradeId,
				LevelState.CurrentLevel
			);
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

void UEquipmentUpgradeSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetNetMode() == NM_Client)
	{
		return;
	}

	if (GetGameInstance() &&
		LoadedWorld != GetGameInstance()->GetWorld())
	{
		return;
	}

	// 새 월드의 액터 생성을 감시한다.
	BindToWorld(LoadedWorld);

	// 새 맵에서도 강화 상태 액터를 반드시 만든다.
	// 강화가 초기화된 경우 빈 배열을 클라이언트에 복제하기 위해 필요하다.
	if (HasServerAuthority())
	{
		GetOrCreateStateActor();
	}

	// 이미 생성된 Pawn이 있다면 바로 적용한다.
	for (TActorIterator<APawn> It(LoadedWorld); It; ++It)
	{
		if (APawn* Pawn = *It)
		{
			ApplySavedUpgradesToActor(Pawn);
		}
	}
}

void UEquipmentUpgradeSubsystem::HandleActorSpawned(AActor* SpawnedActor)
{
	if (!SpawnedActor)
	{
		return;
	}

	if (!BoundWorld.IsValid() ||
		SpawnedActor->GetWorld() != BoundWorld.Get())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(SpawnedActor);
	if (!Pawn)
	{
		return;
	}

	// 새 Pawn이 생성되면 저장된 강화 효과를 다시 적용한다.
	ApplySavedUpgradesToActor(Pawn);
}

void UEquipmentUpgradeSubsystem::BindToWorld(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	if (BoundWorld.Get() == World &&
		ActorSpawnedHandle.IsValid())
	{
		return;
	}

	UnbindFromWorld();

	BoundWorld = World;

	ActorSpawnedHandle =
		World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(
				this,
				&UEquipmentUpgradeSubsystem::HandleActorSpawned
			)
		);
}

void UEquipmentUpgradeSubsystem::UnbindFromWorld()
{
	if (UWorld* World = BoundWorld.Get())
	{
		if (ActorSpawnedHandle.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		}
	}

	ActorSpawnedHandle.Reset();
	BoundWorld.Reset();
}