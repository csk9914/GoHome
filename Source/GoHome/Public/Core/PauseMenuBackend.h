#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PauseMenuBackend.generated.h"

// 확인 팝업이 끝난 뒤 세션을 정리하고 갈 곳
UENUM(BlueprintType)
enum class EPauseLeaveTarget : uint8
{
	Title,
	QuitGame,
};

// 확인 문구를 가르는 역할 — 호스트가 나가면 리슨 서버가 닫혀 참가자 연결도 끊긴다.
UENUM(BlueprintType)
enum class EPauseSessionRole : uint8
{
	Host,
	Participant,
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FPauseLeaveFailedEvent, EPauseLeaveTarget /*Target*/, const FText& /*Reason*/);

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPauseMenuBackend : public UInterface
{
	GENERATED_BODY()
};

/**
 * 인게임 시스템 메뉴(UI/Pause)가 아는 유일한 계약. 메뉴 위젯의 생성·입력 모드·세션 정리는 구현체(AGoHomePlayerController)가 소유한다.
 * 성공한 나가기는 레벨 이동/앱 종료로 끝나므로 실패만 이벤트로 알린다.
 */
class GOHOME_API IPauseMenuBackend
{
	GENERATED_BODY()

public:
	virtual EPauseSessionRole GetSessionRole() const = 0;

	// 닫힘 연출이 끝난 뒤 호출 — 위젯 제거와 게임 입력·커서 복원
	virtual void RequestResume() = 0;

	// 세션 정리가 끝난 뒤에만 이동/종료한다. 이미 진행 중이면 무시.
	virtual void RequestLeave(EPauseLeaveTarget Target) = 0;
	virtual bool IsLeaveInFlight() const = 0;

	virtual FPauseLeaveFailedEvent& OnLeaveFailed() = 0;
};
