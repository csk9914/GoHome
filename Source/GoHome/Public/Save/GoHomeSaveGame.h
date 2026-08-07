

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GoHomeSaveGame.generated.h"

/** 호스트 로컬 세이브: 파티 공유 재화, 구매 완료 업그레이드 목록, 마지막 진행 지점. */
UCLASS()
class GOHOME_API UGoHomeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SharedCurrency = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FName> PurchasedUpgrades;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName LastProgressPoint;
};
