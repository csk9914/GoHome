#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionSubsystem.generated.h"

// ---델리게이트 선언---

// 엔진 전역 델리게이트 이름과의 충돌을 피하기 위해 접두사 'FGoHomeSession'을 사용했습니다.

// 세션 생성 완료 시 호출 (성공 여부) -> 블루프린트 바인딩 가능
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoHomeSessionOnCreateComplete, bool, bWasSuccessful);

// 세션 파괴 완료 시 호출 (성공 여부) -> 블루프린트 바인딩 가능
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoHomeSessionOnDestroyComplete, bool, bWasSuccessful);

// 세션 시작 완료 시 호출 (성공 여부) -> 블루프린트 바인딩 가능
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoHomeSessionOnStartComplete, bool, bWasSuccessful);

// 세션 검색 완료 시 호출 (검색 결과 목록, 성공 여부) -> C++ 전용
DECLARE_MULTICAST_DELEGATE_TwoParams(FGoHomeSessionOnFindComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

// 세션 참가 완료 시 호출 (참가 결과 Enum) -> C++ 전용
DECLARE_MULTICAST_DELEGATE_OneParam(FGoHomeSessionOnJoinComplete, EOnJoinSessionCompleteResult::Type Result);


/**
 * OnlineSubsystemSteam 기반 세션 생성/참가/파괴를 감싼다 (리슨 서버, 호스트가 서버 겸임).
 * 트래블(ServerTravel "?listen" / ClientTravel)은 호출 측(UI/GameInstance)의 책임이다 —
 * 이 서브시스템은 세션 인터페이스 호출과 결과 브로드캐스트까지만 책임진다.
 */

UCLASS()
class GOHOME_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USessionSubsystem();

	// 서브시스템 종료/소멸 시 내부 리소스 정리 (델리게이트 해제 등)
	virtual void Deinitialize() override;

public:
	// 외부 진입점 함수 (Public API)
	// UI나 C++ 코드에서 세션 작업을 시작할 때 호출
	void CreateSession(int32 NumPublicConnections);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

public:
	// ---외부 노출용 델리게이트 인스턴스---
	// 외부(UI Widget, GameInstance 등)에서 이벤트 결과를 구독(AddDynamic/Add)할 때 사용
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGoHomeSessionOnCreateComplete OnCreateComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGoHomeSessionOnDestroyComplete OnDestroyComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGoHomeSessionOnStartComplete OnStartComplete;

	// C++ 전용 델리게이트 (BlueprintAssignable 불가)
	FGoHomeSessionOnFindComplete OnFindComplete;
	FGoHomeSessionOnJoinComplete OnJoinComplete;
	
protected:
	// 내부 콜백 함수 (On~SessionComplete)
	// OnlineSubsystem의 비동기 작업이 끝났을 때 온라인 인터페이스에 의해 실제 내부적으로 호출
	// 내부 처리를 진행한 후, 위 public 델리게이트를 통해 외부(UI 등)로 알림을 보냅니다.
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	// ---엔진 연동 내부 구현---
	// 완료 시 호출할 함수를 바인딩하는 델리게이트
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	
	// 바인딩된 델리게이트를 등록/해제할 때 식별자로 사용하는 고유 핸들(영수증)
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle StartSessionCompleteDelegateHandle;

private:
	// 스팀/LAN 등 실제 온라인 세션 기능을 제공하는 언리얼 엔진 세션 인터페이스 포인터
	IOnlineSessionPtr SessionInterface;
	
	// 세션 설정 및 데이터 공유 스마트 포인터
	// 언리얼 OnlineSubsystem은 내부적으로 TSharedPtr를 사용하므로 해당 생명주기를 유지
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// 예외 처리 및 재시도 상태 변수 (흐름을 제어하기 위한 플래그/변수)
	bool bCreateSessionOnDestroy = false;
	int32 LastNumPublicConnections = 4;
};
