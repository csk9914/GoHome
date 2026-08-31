#pragma once
#include"CoreMinimal.h"
#include "FExpeditionProgress.generated.h"

USTRUCT(BlueprintType)
struct FExpeditionProgress
{
	GENERATED_BODY()
	
	// ** 진행 상황 **
	// 현재 진행된 탐사 라운드
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentRound = 0;

	// 마지막 라운드 번호
	UPROPERTY(BlueprintReadOnly)
	int32 FinalRound = 0;
	
	// 현재 자금
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentFunds = 0;
	
	// 누적 스트라이크
	UPROPERTY(BlueprintReadOnly)
	int32 StrikeCount = 0;

	// 다음 체크포인트 턴 번호
	UPROPERTY(BlueprintReadOnly)
	int32 NextCheckPointRound = 0;
	
	// 다음 체크 포인트 목표액
	UPROPERTY(BlueprintReadOnly)
	int32 NextCheckPointQuota = 0;
};
