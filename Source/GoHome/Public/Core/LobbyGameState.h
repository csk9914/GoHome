// 

#pragma once

#include "CoreMinimal.h"
#include "GoHomeGameState.h"
#include "LobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedZoneChanged, FName, NewZoneId);

// 로비 상시 자금 표시 HUD가 바인딩 — 바인딩 직후 GetCurrentFunds()로 한 번 당겨오도록
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentFundsChanged, int32, NewCurrentFunds);

class UExpeditionZoneDataAsset;

/**
 * 
 */
UCLASS()
class GOHOME_API ALobbyGameState : public AGoHomeGameState
{
	GENERATED_BODY()
	
public:
	ALobbyGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서만 호출
	UFUNCTION(BlueprintCallable, Category = "Zone")
	void SetSelectedZone(FName ZoneId);

	UFUNCTION(BlueprintPure, Category = "Zone")
	const UExpeditionZoneDataAsset* GetSelectedZone() const;

	// UI 갱신용 델리게이트 브로드 캐스트
	UFUNCTION()
	void OnRep_SelectedZone();

	UFUNCTION()
	void OnRep_CurrentFunds();

	UFUNCTION(BlueprintPure, Category = "Save")
	int32 GetCurrentFunds() const { return CurrentFunds; }

	// 서버 전용. 세이브의 보유 자금을 복제 필드에 싣는다(BeginPlay에서 1회).
	void SetCurrentFunds(int32 InCurrentFunds);

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FOnSelectedZoneChanged OnSelectedZoneChanged;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FOnCurrentFundsChanged OnCurrentFundsChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zone")
	TArray<TObjectPtr<UExpeditionZoneDataAsset>> AvailableZones;

	UPROPERTY(ReplicatedUsing=OnRep_SelectedZone, BlueprintReadOnly, Category="Zone")
	FName SelectedZoneId = NAME_None;

	// 로비 상시 자금 표시용(세이브 CurrentFunds 미러, BeginPlay 1회 세팅).
	UPROPERTY(ReplicatedUsing=OnRep_CurrentFunds, BlueprintReadOnly, Category="Save", meta = (AllowPrivateAccess = "true"))
	int32 CurrentFunds = 0;


};
