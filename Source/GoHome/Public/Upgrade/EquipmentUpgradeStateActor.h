#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Upgrade/EquipmentUpgradeTypes.h"
#include "EquipmentUpgradeStateActor.generated.h"

// 강화 레벨이 바뀌었을 때 UI나 Subsystem이 다시 갱신되게 알려주는 신호
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentUpgradeStateChanged);

UCLASS()
class GOHOME_API AEquipmentUpgradeStateActor : public AActor
{
	GENERATED_BODY()

public:
	AEquipmentUpgradeStateActor();

	// 클라이언트에서 강화 레벨 복제 받았을 때 UI 갱신용으로 쓰는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Equipment Upgrade")
	FOnEquipmentUpgradeStateChanged OnEquipmentUpgradeStateChanged;

	// 특정 강화 ID의 현재 레벨을 가져온다.
	// 없으면 기본 1레벨로 본다.
	int32 GetUpgradeLevel(FName UpgradeId) const;

	// 전체 강화 레벨 배열을 Subsystem이 읽을 수 있게 한다.
	const TArray<FEquipmentUpgradeLevelState>& GetUpgradeLevels() const { return UpgradeLevels; }

	// 서버에서 강화 레벨을 바꿀 때 사용한다.
	// 클라이언트는 직접 바꾸면 안 된다.
	bool SetUpgradeLevel(FName UpgradeId, int32 NewLevel);

protected:
	virtual void BeginPlay() override;

	// Replication에 UpgradeLevels를 등록하기 위해 필요하다.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// UpgradeLevels가 서버에서 클라이언트로 복제됐을 때 자동 호출된다.
	UFUNCTION()
	void OnRep_UpgradeLevels();

private:
	// 모든 플레이어가 공유하는 강화 레벨 상태.
	// 예: OxygenCapacity = 2, CarryWeightLimit = 1
	UPROPERTY(ReplicatedUsing = OnRep_UpgradeLevels)
	TArray<FEquipmentUpgradeLevelState> UpgradeLevels;
};