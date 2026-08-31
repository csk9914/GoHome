

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
	// 자금, (납품 +, 강화 or 구매 -
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 CurrentFunds = 0;

	// 이번 라운드 누적 납품액, 판정 후 0으로 리셋
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 CurrentRoundDeliveredValue = 0;
	
	// 완료한 라운드 수
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 CurrentRound = 0;
	
	// 할당량 누적 미달 횟수(연속 아님, 성공해도 리셋 안 됨). 
	// 3회(3스트라이크) 도달 시 게임오버로 세이브 전체 초기화
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 QuotaMissCount = 0;

	// 강화
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FName> PurchasedUpgrades;

	// 마지막 도달 진행 지점
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName LastProgressPoint;
	
};
