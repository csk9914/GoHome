// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GoHomePlayerController.generated.h"

class UEquipmentUpgradeDataAsset;
class AGameStateBase;

/**
 *
 */
UCLASS()
class GOHOME_API AGoHomePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 탐사 HUD 묶음(WBP_HUD) 생성/파괴 지점. 실제 CreateWidget + AddToViewport 는 BP_GoHomePlayerController 가 한다.
	// Ready  : 로컬 컨트롤러 + AExplorationGameState 유효할 때 1회 (탐사 레벨 진입).
	// Teardown: 탐사 레벨을 벗어날 때 1회 (로비 복귀 / 접속 종료).
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnExplorationHUDReady();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnExplorationHUDTeardown();

	UFUNCTION(BlueprintImplementableEvent, Category="UI")
	void OnOpenSelectZone();
	
	UFUNCTION(Server, Reliable, BlueprintCallable)                                                                  
	void Server_SelectZone(FName ZoneId);
	
	// UI 실제 생성/뷰포트 추가는 BP에서 (WBP_ZoneSelect 참조는 C++이 몰라도 되므로)                  
	UFUNCTION(Client, Reliable)                                                                       
	void Client_OpenSelectZone();

	// 강화 관련
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnOpenEquipmentUpgrade();

	UFUNCTION(Client, Reliable)
	void Client_OpenEquipmentUpgrade();

	// 클라이언트 UI에서 누른 강화 요청을 서버로 전달한다.
	// 실제 처리 로직은 EquipmentUpgradeSubsystem에서 담당한다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment Upgrade")
	void Server_RequestEquipmentUpgrade(UEquipmentUpgradeDataAsset* UpgradeData);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_Pawn() override;
	virtual void SetPawn(APawn* InPawn) override;

private:
	// 로컬 컨트롤러에서 현재 월드의 GameState 로 탐사 레벨 여부를 판정해 Ready/Teardown 을 엣지에서 1회씩 쏜다.
	void RefreshExplorationHUD();

	// GameState 가 Pawn 보다 늦게 복제되는 경우를 위해 월드마다 GameStateSetEvent 에 재바인딩.
	void BindGameStateSetEvent();
	void HandleGameStateSet(AGameStateBase* NewGameState);

	bool bExplorationHUDActive = false;
	TWeakObjectPtr<UWorld> BoundGameStateWorld;
};
