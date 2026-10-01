// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Shop/ItemShopTypes.h"
#include "GoHomePlayerController.generated.h"

class UEquipmentUpgradeDataAsset;
class AGameStateBase;
class UUserWidget;

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

	UFUNCTION(Client, Reliable)
	void Client_RefreshUpgradeFunds(int32 InCurrentFunds);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUpgradeCurrentFundsChanged(int32 InCurrentFunds);

	// 클라이언트 UI에서 누른 강화 요청을 서버로 전달한다.
	// 실제 처리 로직은 EquipmentUpgradeSubsystem에서 담당한다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment Upgrade")
	void Server_RequestEquipmentUpgrade(UEquipmentUpgradeDataAsset* UpgradeData);

	// 상점 관련 - UI의 구매 주문서를 서버로 전달한다.
	// 실제 상점 처리는 ItemShopSubsystem이 담당한다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Item Shop")
	void Server_RequestShopPurchase(FItemShopPurchaseRequest Request);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_Pawn() override;
	virtual void SetPawn(APawn* InPawn) override;

	// 로비·탐사 공통 상시 진행도 HUD(라운드·다음 관문·보유 자금, 탐사맵이면 할당량). BP에서 위젯 클래스를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ProgressHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	int32 ProgressHUDZOrder = 0;

private:
	// 로컬 컨트롤러에서 현재 월드의 GameState 로 탐사 레벨 여부를 판정해 Ready/Teardown 을 엣지에서 1회씩 쏜다.
	void RefreshExplorationHUD();

	// 로비/탐사 GameState 가 잡힐 때마다 진행도 HUD 를 그 GameState 기준으로 (재)생성한다.
	// seamless travel 은 컨트롤러를 살려 둔 채 월드만 바꾸므로 BeginPlay 1회 생성으로는 탐사맵에서 사라진다.
	void RefreshProgressHUD();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ProgressHUD;

	// ProgressHUD 가 생성될 때의 GameState — 다르면(새 월드) 다시 만든다
	TWeakObjectPtr<AGameStateBase> ProgressHUDGameState;

	// GameState 가 Pawn 보다 늦게 복제되는 경우를 위해 월드마다 GameStateSetEvent 에 재바인딩.
	void BindGameStateSetEvent();
	void HandleGameStateSet(AGameStateBase* NewGameState);

	bool bExplorationHUDActive = false;
	TWeakObjectPtr<UWorld> BoundGameStateWorld;
};
