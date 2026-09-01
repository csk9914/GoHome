// 

#pragma once

#include "CoreMinimal.h"
#include "FailReason.h"
#include "GoHomeGameMode.h"
#include "ExplorationGameMode.generated.h"





/**
 * 
 */
UCLASS()
class GOHOME_API AExplorationGameMode : public AGoHomeGameMode
{
	GENERATED_BODY()
	
public:
	AExplorationGameMode();
	void HandleFail(EFailReason Reason);
	
	virtual void RestartPlayer(AController* NewPlayer) override;

	
private:
	void TrackPawnDeath(APawn* Pawn);
	void HandlePawnDeath(TWeakObjectPtr<APawn> DeadPawn);
	void CheckAllDead();
	
	void ReturnToLobby();
	
private:
	
	// 구조 실패 화면 노출 시간
	UPROPERTY(EditDefaultsOnly, Category = "Expedition")
	float AutoReturnDelay = 8.f;
	
	// 정산에 넘길 명단
	TSet<FString> CasualtyNames;
	
	// 중복 구독 방지용
	TSet<TWeakObjectPtr<UActorComponent>> TrackedNotifiers;
	
	// 사망 폰 집계
	TSet<TWeakObjectPtr<APawn>> DeadPawns;
	
	FTimerHandle AutoReturnTimer;
	
	bool bFailHandled = false;
	
	
};
