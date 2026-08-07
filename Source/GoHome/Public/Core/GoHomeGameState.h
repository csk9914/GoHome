

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/ExpeditionState.h"
#include "Core/FailReason.h"
#include "GoHomeGameState.generated.h"

class UDockingDoorComponent;
class APlayerState;

/**
 * 탐사 진행 단계만 책임진다 (도킹 문 상태는 UDockingDoorComponent가 별도 소유).
 */
UCLASS()
class GOHOME_API AGoHomeGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGoHomeGameState();

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Expedition")
	EExpeditionState State = EExpeditionState::Lobby;

	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnExpeditionStateChanged OnStateChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Docking Door")
	TObjectPtr<UDockingDoorComponent> DockingDoorComponent;

	UFUNCTION(BlueprintCallable, Category = "Expedition")
	void AddDeliveredValue(int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Expedition")
	void Fail(EFailReason Reason);

	void OnPlayerRemovedFromParty(APlayerState* PlayerState);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_State();

	UPROPERTY()
	TSet<TObjectPtr<APlayerState>> RemovedFromParty;
};
