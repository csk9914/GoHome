// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GoHomePlayerController.generated.h"

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
	
	UFUNCTION(Server, Reliable)                                                                       
	void Server_SelectZone(FName ZoneId);
	
	// UI 실제 생성/뷰포트 추가는 BP에서 (WBP_ZoneSelect 참조는 C++이 몰라도 되므로)                  
	UFUNCTION(Client, Reliable)                                                                       
	void Client_OpenSelectZone();
};
