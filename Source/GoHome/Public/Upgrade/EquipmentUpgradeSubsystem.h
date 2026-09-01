#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Upgrade/EquipmentUpgradeTypes.h"
#include "EquipmentUpgradeSubsystem.generated.h"

class UEquipmentUpgradeDataAsset;
class AActor;
class APlayerController;
class AEquipmentUpgradeStateActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentUpgradesChanged);

UCLASS(BlueprintType)
class GOHOME_API UEquipmentUpgradeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Equipment Upgrade")
	FOnEquipmentUpgradesChanged OnEquipmentUpgradesChanged;

	UFUNCTION(BlueprintPure, Category = "Equipment Upgrade")
	int32 GetUpgradeLevel(FName UpgradeId) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment Upgrade")
	FEquipmentUpgradePreview BuildUpgradePreview(UEquipmentUpgradeDataAsset* UpgradeData, float BaseValue) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment Upgrade")
	EEquipmentUpgradeRequestResult UpgradeOnce(UEquipmentUpgradeDataAsset* UpgradeData);

	// 현재 강화 레벨의 효과를 특정 액터에게 실제로 적용한다.
	// 예: 산소 강화라면 TargetActor의 OxygenComponent를 찾아서 MaxOxygenBonus를 세팅한다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment Upgrade")
	EEquipmentUpgradeRequestResult ApplyUpgradeToActor(UEquipmentUpgradeDataAsset* UpgradeData, AActor* TargetActor);

	// Subsystem에 클라이언트 동기화 함수 추가
	void SetUpgradeLevelForSync(FName UpgradeId, int32 NewLevel);

	// 누가 강화 요청했는지 확인하기 위해서
	// 나중에 코인 / 권한 / 거리 체크 붙일 자리
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment Upgrade")
	EEquipmentUpgradeRequestResult RequestUpgrade(APlayerController* RequestingPlayer, UEquipmentUpgradeDataAsset* UpgradeData);

	// StateActor가 BeginPlay 때 Subsystem에 자신을 등록한다.
	// 서버/클라이언트 모두 여기로 들어올 수 있다.
	void RegisterStateActor(AEquipmentUpgradeStateActor* StateActor);

protected:
	UPROPERTY()
	TArray<FEquipmentUpgradeLevelState> UpgradeLevels;

private:
	bool HasServerAuthority() const;
	FEquipmentUpgradeLevelState* FindMutableLevelState(FName UpgradeId);
	const FEquipmentUpgradeLevelState* FindLevelState(FName UpgradeId) const;

	// 현재 월드의 강화 상태 액터.
	// 서버 값을 클라이언트 UI로 복제하는 역할이다.
	UPROPERTY()
	TObjectPtr<AEquipmentUpgradeStateActor> CachedStateActor;

	// StateActor가 있으면 가져오고, 서버에서 없으면 자동 생성한다.
	// 클라이언트는 직접 생성하지 않는다.
	AEquipmentUpgradeStateActor* GetOrCreateStateActor();

	// StateActor의 복제 값이 바뀌었을 때 Subsystem 내부 배열도 맞춘다.
	UFUNCTION()
	void HandleStateActorChanged();

	// StateActor 안의 UpgradeLevels를 Subsystem으로 복사한다.
	void SyncLevelsFromStateActor();
};