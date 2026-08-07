

#pragma once

#include "CoreMinimal.h"
#include "ExpeditionState.generated.h"

/** GameState 상태 머신: 로비 -> 탐사 지역 선택 -> 출발 -> 탐사 -> 복귀 -> 정산 (또는 실패). */
UENUM(BlueprintType)
enum class EExpeditionState : uint8
{
	Lobby,
	ZoneSelect,
	Departure,
	Exploration,
	Return,
	Settlement,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExpeditionStateChanged, EExpeditionState, NewState);
