// 


#include "Core/GoHomePlayerController.h"

#include "Core/LobbyGameState.h"
#include "Core/ExplorationGameState.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Upgrade/EquipmentUpgradeSubsystem.h"


void AGoHomePlayerController::BeginPlay()
{
	Super::BeginPlay();
	BindGameStateSetEvent();
	RefreshExplorationHUD();
}

void AGoHomePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bExplorationHUDActive)
	{
		bExplorationHUDActive = false;
		OnExplorationHUDTeardown();
	}
	Super::EndPlay(EndPlayReason);
}

void AGoHomePlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	BindGameStateSetEvent();
	RefreshExplorationHUD();
}

void AGoHomePlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	BindGameStateSetEvent();
	RefreshExplorationHUD();
}

void AGoHomePlayerController::BindGameStateSetEvent()
{
	UWorld* World = GetWorld();
	if (!World || BoundGameStateWorld.Get() == World)
	{
		return;
	}

	BoundGameStateWorld = World;
	World->GameStateSetEvent.AddUObject(this, &AGoHomePlayerController::HandleGameStateSet);
}

void AGoHomePlayerController::HandleGameStateSet(AGameStateBase* /*NewGameState*/)
{
	RefreshExplorationHUD();
}

void AGoHomePlayerController::RefreshExplorationHUD()
{
	if (!IsLocalController())
	{
		return;
	}

	const bool bWantHUD = GetWorld() && GetWorld()->GetGameState<AExplorationGameState>() != nullptr;
	if (bWantHUD == bExplorationHUDActive)
	{
		return;
	}

	bExplorationHUDActive = bWantHUD;
	if (bWantHUD)
	{
		OnExplorationHUDReady();
	}
	else
	{
		OnExplorationHUDTeardown();
	}
}

void AGoHomePlayerController::Server_SelectZone_Implementation(FName ZoneId)
{
	if (ALobbyGameState* LobbyGameState = GetWorld()->GetGameState<ALobbyGameState>())
	{
		LobbyGameState->SetSelectedZone(ZoneId);
	}
}

void AGoHomePlayerController::Client_OpenSelectZone_Implementation()
{
	// 실제 위젯 생성/표시는 BlueprintImplementableEvent로 열어두거나                                 
	// 여기서 바로 CreateWidget 호출 — 컨벤션상 BP 확장 지점 열어두는 쪽 권장
	OnOpenSelectZone();
}

void AGoHomePlayerController::Client_OpenEquipmentUpgrade_Implementation()
{
	OnOpenEquipmentUpgrade();
}

void AGoHomePlayerController::Client_RefreshUpgradeFunds_Implementation(
	int32 InCurrentFunds)
{
	OnUpgradeCurrentFundsChanged(InCurrentFunds);
}

// PlayerController는 클라이언트 UI 요청을 서버로 넘기는 통로만 맡는다.
// 실제 강화 처리는 EquipmentUpgradeSubsystem에 위임한다.
void AGoHomePlayerController::Server_RequestEquipmentUpgrade_Implementation(UEquipmentUpgradeDataAsset* UpgradeData)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UEquipmentUpgradeSubsystem* UpgradeSubsystem = GameInstance->GetSubsystem<UEquipmentUpgradeSubsystem>();
	if (!UpgradeSubsystem)
	{
		return;
	}

	int32 CurrentFunds = 0;

	const EEquipmentUpgradeRequestResult UpgradeResult =
		UpgradeSubsystem->RequestUpgrade(
			this,
			UpgradeData,
			CurrentFunds);

	if (UpgradeResult != EEquipmentUpgradeRequestResult::Succeeded)
	{
		return;
	}

	// 강화 후 최신 코인을 메인 HUD용 GameState 미러에 반영한다.
	if (UWorld* World = GetWorld())
	{
		if (AExplorationGameState* ExplorationGameState =
			World->GetGameState<AExplorationGameState>())
		{
			ExplorationGameState->SetCurrentFunds(CurrentFunds);
		}
	}

	// 강화 UI에도 최신 코인을 전달한다.
	Client_RefreshUpgradeFunds(CurrentFunds);
}