

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
	/** 강화 구매로 소비되는 잔액. */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SharedCurrency = 0;

	/** 정산 시마다 누적만 되고 줄지 않는 누적 획득액 — 엔딩(탈출 성공) 판정 기준(02문서 13-2절). */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 TotalRecoveredValue = 0;

	/** 현재 라운드에 요구되는 목표 점수(할당량). 라운드마다 증가한다(02문서 13-2절). */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 CurrentQuotaTarget = 0;

	/** 할당량 연속 미달 횟수. N회(TBD) 도달 시 게임오버로 세이브 전체 초기화(02문서 13-2절). */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 ConsecutiveQuotaMisses = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FName> PurchasedUpgrades;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName LastProgressPoint;
};
