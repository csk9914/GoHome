#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/TitleTypes.h"
#include "Data/FExpeditionProgress.h"
#include "TitleBackend.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTitleBackend : public UInterface
{
	GENERATED_BODY()
};

/**
 * 타이틀 UI가 아는 유일한 계약. UI는 Core 구체 클래스 대신 이 인터페이스로 세션·세이브 흐름을 요청/구독한다.
 * 요청 함수는 결과를 반환하지 않고, 결과는 GetTitleEvents()의 이벤트로만 알린다.
 */
class GOHOME_API ITitleBackend
{
	GENERATED_BODY()

public:
	virtual FTitleBackendEvents& GetTitleEvents() = 0;

	// 구독 직후 초기 표시용 — 마지막 검색 상태
	virtual const FTitleSearchSnapshot& GetSearchSnapshot() const = 0;

	virtual bool HasResumableProgress() const = 0;
	virtual FExpeditionProgress GetProgressSummary() const = 0;

	virtual bool IsHostInFlight() const = 0;
	virtual bool IsJoinInFlight() const = 0;

	virtual void RequestHost(ETitleHostMode Mode) = 0;

	// 검색이 진행 중이면 무시한다
	virtual void RequestRefresh() = 0;

	// Generation이 최신 스냅샷과 다르거나 이미 참가 중이면 false (요청 안 함)
	virtual bool RequestJoin(int32 Generation, int32 SearchIndex) = 0;
};
