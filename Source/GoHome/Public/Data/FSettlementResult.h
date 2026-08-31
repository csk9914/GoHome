
#pragma once
#include "CoreMinimal.h"
#include "FExpeditionProgress.h"
#include "FSettlementResult.generated.h"

// 정산 결과 - UI가 어떤 화면을 띄울지 결정
UENUM(BlueprintType)
enum class ESettlementOutcome : uint8{
	Normal,					// 일반 라운드
	CheckPointPassed,		// 누적 소지금 체크 포인트 달성 
	GameOver_Strike,		// 3 스트라이크 게임 오버
	GameOver_CheckPoint,	// 누적 소지금 미달 게임 오버
	Ending,					// 9라운드 목표 달성
};

// UI 넘기는 정산 데이터
USTRUCT(BlueprintType)
struct FSettlementResult
{
	GENERATED_BODY()
	
	// 완전 실패 -> 정산표 대신 임무 실패 화면
	UPROPERTY(BlueprintReadOnly)
	bool bForfeited = false;
	// 정산 결과에 따른 상태 표현 (통과, 게임오버, 엔딩)
	UPROPERTY(BlueprintReadOnly)
	ESettlementOutcome Outcome = ESettlementOutcome::Normal;
	
	
	// 실제 납품액 (패널티 적용되지 않은 값)
	UPROPERTY(BlueprintReadOnly)
	int32 RoundDeliveredValue = 0;
	// 맵의 할당량 기준
	UPROPERTY(BlueprintReadOnly)
	int32 MapQuota = 0;
	
	
	// 사망자 이름 목록 
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> CasualtyNames;	
	// 사망자 차감액 (사망자 수 * EconomyConfig.CasualtyFee)
	UPROPERTY(BlueprintReadOnly)
	int32 CasualtyPenalty =0;
	// 순이득 (유효납품 - 사망 패널티)
	UPROPERTY(BlueprintReadOnly)
	int32 NetGain = 0;
	
	
	// 체크 포인트 라운드인지
	UPROPERTY(BlueprintReadOnly)
	bool bWasCheckPoint = false;
	// 이번 체크 포인트 목표액
	UPROPERTY(BlueprintReadOnly)
	int32 CheckPointQuota = 0;	
	// ── 정산 반영 후 현재 상태 (로비 UI 와
	UPROPERTY(BlueprintReadOnly)
	FExpeditionProgress ExpeditionProgress;

};
