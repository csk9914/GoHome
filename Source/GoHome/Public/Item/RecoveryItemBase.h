#pragma once

#include "CoreMinimal.h"
#include "Item/UsableItemBase.h"
#include "RecoveryItemBase.generated.h"

class APawn;
class UPointLightComponent;

UENUM(BlueprintType)
enum class ERecoveryItemType : uint8
{
	Oxygen UMETA(DisplayName = "Oxygen"),
	Health UMETA(DisplayName = "Health")
};

/**
 * 회복 아이템 공통 동작.
 * 맵에 놓여 있을 때 E 상호작용으로 즉시 사용하고, 인벤토리에 들어간 경우 좌클릭 사용도 지원한다.
 * 스크랩 데이터와 스폰 시스템은 변경하지 않는다.
 */
UCLASS()
class GOHOME_API ARecoveryItemBase : public AUsableItemBase
{
	GENERATED_BODY()

public:
	ARecoveryItemBase();

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override;
	virtual bool CanUse() const override;
	virtual void ServerUseSpecialAction() override;
	virtual bool IsDeliverable() const override { return false; }
	virtual FText GetInteractionPromptText_Implementation() const override;

	// 상점 연결 시 이 함수를 호출하면 회복 효과 없이 인벤토리에 지급할 수 있다.
	// 현재 상점은 일반 OnInteract를 지급 경로로 쓰므로, 추후 작은 연결 변경이 필요하다.
	bool GiveToInventory(APawn* TargetPawn);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery")
	ERecoveryItemType RecoveryType = ERecoveryItemType::Oxygen;

	// 밸런스 값은 BP 자식에서 설정한다. 기본값 0은 미설정 아이템의 오작동을 막는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery", meta = (ClampMin = "0.0", ToolTip = "아이템을 사용할 때 회복할 산소 또는 HP 양입니다. 기본값 0이면 회복하지 않습니다."))
	float RecoveryAmount = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Recovery|Visual")
	TObjectPtr<UPointLightComponent> PickupGlow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery|Visual", meta = (ToolTip = "아이템이 은은한 빛을 내도록 켭니다."))
	bool bEnablePickupGlow = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery|Visual", meta = (ClampMin = "0.0", ToolTip = "아이템 주변 빛의 밝기입니다."))
	float PickupGlowIntensity = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery|Visual", meta = (ClampMin = "0.0", ToolTip = "아이템 주변 빛이 닿는 거리입니다."))
	float PickupGlowRadius = 140.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery|Visual", meta = (ToolTip = "산소 회복 아이템의 빛 색상입니다."))
	FLinearColor OxygenGlowColor = FLinearColor(0.05f, 0.65f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery|Visual", meta = (ToolTip = "HP 회복 아이템의 빛 색상입니다."))
	FLinearColor HealthGlowColor = FLinearColor(0.25f, 1.f, 0.35f, 1.f);

private:
	bool CanRestorePawn(const APawn* TargetPawn) const;
	bool ApplyRecovery(APawn* TargetPawn);
	bool ConsumeForPawn(APawn* TargetPawn, bool bRequireInventory);
};
