#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Core/ExpeditionState.h"
#include "Core/FailReason.h"
#include "GoHomeGameState.generated.h"

class UDockingDoorComponent;
class APlayerState;

// 현재/전체 라운드 진행도가 바뀔 때 브로드캐스트 — 로비/탐사 공통 상시 라운드 표기 HUD가 바인딩
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoundProgressChanged, int32, CurrentRound, int32, FinalRound);

/**
 * 탐사 진행 단계만 책임진다 (도킹 문 상태는 UDockingDoorComponent가 별도 소유).
 */
UCLASS()
class GOHOME_API AGoHomeGameState : public AGameState
{
	GENERATED_BODY()

public:
	AGoHomeGameState();

	UFUNCTION(BlueprintCallable, Category = "Expedition")
	void AddDeliveredValue(int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Expedition")
	void Fail(EFailReason Reason);

	void OnPlayerRemovedFromParty(APlayerState* PlayerState);

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_State();

	UFUNCTION()
	void OnRep_RoundProgress();

public:
	UFUNCTION(BlueprintCallable, Category="Expedition")
	void SetState(EExpeditionState NewState);

	UFUNCTION(BlueprintPure, Category = "Expedition")
	EExpeditionState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category="Docking Door")
	UDockingDoorComponent* GetDockingDoorComponent() const { return DockingDoorComponent; }

	// 서버 전용. 세이브의 완료 라운드 수/전체 라운드 수를 복제 필드에 싣는다(BeginPlay에서 1회, SaveSubsystem::BuildProgress 기반).
	void SetRoundProgress(int32 InCurrentRound, int32 InFinalRound);

	UFUNCTION(BlueprintPure, Category = "Expedition")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "Expedition")
	int32 GetFinalRound() const { return FinalRound; }

	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnExpeditionStateChanged OnStateChanged;

	// 로비/탐사 공통 상시 라운드 표기 HUD가 바인딩 — 바인딩 직후 Get*()로 한 번 당겨오도록
	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnRoundProgressChanged OnRoundProgressChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Expedition")
	EExpeditionState CurrentState = EExpeditionState::Lobby;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Docking Door")
	TObjectPtr<UDockingDoorComponent> DockingDoorComponent;

	// 완료된 라운드 수(세이브 CurrentRound 미러). 표시용 "현재 라운드"는 위젯 쪽에서 +1.
	UPROPERTY(ReplicatedUsing = OnRep_RoundProgress, BlueprintReadOnly, Category = "Expedition", meta = (AllowPrivateAccess = "true"))
	int32 CurrentRound = 0;

	// 전체 라운드 수(마지막 체크포인트 Round, EconomyConfig 기준).
	UPROPERTY(ReplicatedUsing = OnRep_RoundProgress, BlueprintReadOnly, Category = "Expedition", meta = (AllowPrivateAccess = "true"))
	int32 FinalRound = 0;

private:
	UPROPERTY()
	TSet<TObjectPtr<APlayerState>> RemovedFromParty;
};
