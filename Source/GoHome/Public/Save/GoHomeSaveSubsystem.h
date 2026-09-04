#pragma once

#include "CoreMinimal.h"
#include "Data/FSettlementResult.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GoHomeSaveSubsystem.generated.h"

class UEconomyConfigDataAsset;
class UWorld;
class UGoHomeSaveGame;
enum class EExpeditionState : uint8;
struct FCheckPoint;

/**
 * 트래블 간 유지되는 GameInstanceSubsystem. GameState는 맵 이동마다 새로 스폰되므로
 * 이 서브시스템이 매 레벨 GameState::BeginPlay에서 OnStateChanged에 재구독한다.
 * NewState == Lobby일 때 저장하며, 구독 직후 현재 상태가 이미 Lobby면 즉시 저장한다
 * (로비 맵 재진입 시 엣지 트리거 누락 방지).
 */

UCLASS()
class GOHOME_API UGoHomeSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& CollectionBase) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveToDisk();

	// 납품 계산. 반영 후 이번 라운드 납품 누적액을 반환(GameState 복제 미러용).
	int32 AccumulateDeliveredValue(int32 Value);

	// 라운드 종료 정산
	FSettlementResult FinalizeRound(bool bForfeited, const TArray<FString>& CasualtyNames);

	// 세이브의 상태를 리플리케이트 가능한 평면 struct로 복사
	FExpeditionProgress BuildProgress() const;

	UFUNCTION(BlueprintPure, Category = "Save")
	const UGoHomeSaveGame* GetSaveGame() const { return SaveGame; };

	// 현재 보유 자금(GameState 복제 미러용). 세이브 없으면 0.
	int32 GetCurrentFunds() const;

	// 출발 시 호출해서 값을 초기화
	void SetTargetMapQuota(int32 Quota) { CurrentMapQuota = Quota; };

private:
	void ResetSave();
	ESettlementOutcome DetermineOutcome(const FCheckPoint* CheckPoint, int32 CompletedRound) const;

protected:
	UFUNCTION()
	void OnExpeditionStateChanged(EExpeditionState NewState);

	void OnPostLoadMap(UWorld* LoadedWorld);

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	TObjectPtr<UGoHomeSaveGame> SaveGame;

private:
	FDelegateHandle PostLoadMapHandle;

	UPROPERTY()
	TObjectPtr<UEconomyConfigDataAsset> EconomyConfig;

	int32 CurrentMapQuota = 0;
};
