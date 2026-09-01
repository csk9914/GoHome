// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GoHomePlayerController.generated.h"

class UEquipmentUpgradeDataAsset;

/**
 * 
 */
UCLASS()
class GOHOME_API AGoHomePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
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
};
