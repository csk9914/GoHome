#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Core/ExpeditionState.h"
#include "Core/FailReason.h"
#include "GoHomeGameState.generated.h"

class UDockingDoorComponent;
class APlayerState;

/**
 * 탐사 진행 단계만 책임진다 (도킹 문 상태는 UDockingDoorComponent가 별도 소유).
 */
UCLASS()
class GOHOME_API AGoHomeGameState : public AGameState
{
	GENERATED_BODY()

public:
	AGoHomeGameState();

	UFUNCTION(BlueprintCallable, Category="Expedition")
	void SetState(EExpeditionState NewState);

	UFUNCTION(BlueprintCallable, Category = "Expedition")
	void AddDeliveredValue(int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Expedition")
	void Fail(EFailReason Reason);

	void OnPlayerRemovedFromParty(APlayerState* PlayerState);

	UFUNCTION(BlueprintPure, Category="Docking Door")
	UDockingDoorComponent* GetDockingDoorComponent() const { return DockingDoorComponent; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_State();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Expedition")
	EExpeditionState CurrentState = EExpeditionState::Lobby;

	UPROPERTY(BlueprintAssignable, Category = "Expedition")
	FOnExpeditionStateChanged OnStateChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Docking Door")
	TObjectPtr<UDockingDoorComponent> DockingDoorComponent;

private:
	UPROPERTY()
	TSet<TObjectPtr<APlayerState>> RemovedFromParty;
};
