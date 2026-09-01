#pragma once

#include "CoreMinimal.h"
#include "EquipmentUpgradeTypes.generated.h"

// 강화가 어떤 능력치를 올리는지 구분하는 타입.
// 예: 산소 강화인지, 무게 강화인지 구분할 때 쓴다.
UENUM(BlueprintType)
enum class EEquipmentUpgradeEffectType : uint8
{
	None UMETA(DisplayName = "None"),
	OxygenCapacity UMETA(DisplayName = "Oxygen Capacity"),
	CarryWeightLimit UMETA(DisplayName = "Carry Weight Limit")
};


// 강화 요청 결과.
// 버튼을 눌렀을 때 성공했는지, 왜 실패했는지 UI에 알려줄 때 쓴다.
UENUM(BlueprintType)
enum class EEquipmentUpgradeRequestResult : uint8
{
	Succeeded UMETA(DisplayName = "Succeeded"),
	UpgradeNotFound UMETA(DisplayName = "Upgrade Not Found"),
	AlreadyMaxLevel UMETA(DisplayName = "Already Max Level"),
	NotEnoughCurrency UMETA(DisplayName = "Not Enough Currency"),
	InvalidRequester UMETA(DisplayName = "Invalid Requester"),
	NoEffectReceiver UMETA(DisplayName = "No Effect Receiver")
};


// 특정 강화의 현재 레벨.
// 지금은 Subsystem 임시 저장용, 나중에는 SaveGame 저장용으로도 쓸 수 있다.
USTRUCT(BlueprintType)
struct GOHOME_API FEquipmentUpgradeLevelState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	FName UpgradeId = NAME_None;

	// 현재 강화 레벨.
	// 예: 1이면 기본, 2면 한 번 강화됨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade", meta = (ClampMin = "1"))
	int32 CurrentLevel = 1;
};


// UI 카드에 보여줄 데이터 묶음.
// 강화 UI가 이 구조체 하나를 받아서 글자들을 채우게 만들 수 있다.
USTRUCT(BlueprintType)
struct GOHOME_API FEquipmentUpgradePreview
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	FName UpgradeId = NAME_None;

	// 카드 제목.
	// 예: "산소 탱크 업그레이드"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	FText UpgradeName;

	// 능력치 이름.
	// 예: "용량", "무게"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	FText StatLabel;

	// 현재 수치 표시.
	// 예: "15칸"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	FText CurrentValueText;

	// 다음 강화 후 수치 표시.
	// 예: "18칸"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	FText NextValueText;

	// 비용 표시.
	// 예: "150 CREDITS"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	FText CostText;

	// 현재 강화 레벨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	int32 CurrentLevel = 1;

	// 최대 강화 레벨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	int32 MaxLevel = 1;

	// 이미 최대 강화인지 여부.
	// true면 버튼을 "최대 강화"로 바꾸거나 비활성화할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Upgrade")
	bool bIsMaxLevel = false;
};
