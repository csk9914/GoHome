// 


#include "Core/GoHomePlayerController.h"

#include "Core/LobbyGameState.h"

#include "Engine/GameInstance.h"
#include "Upgrade/EquipmentUpgradeSubsystem.h"

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

	UpgradeSubsystem->RequestUpgrade(this, UpgradeData);
}