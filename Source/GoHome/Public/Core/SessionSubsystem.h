

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionSubsystem.generated.h"

// 이름 앞에 GoHomeSession을 붙인다 — 접두사 없는 FOnCreateSessionComplete 등은
// 엔진 OnlineSessionDelegates.h/OnlineSessionInterface.h에 이미 전역으로 선언돼 있어 재정의 충돌이 난다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoHomeSessionOnCreateComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoHomeSessionOnDestroyComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoHomeSessionOnStartComplete, bool, bWasSuccessful);

// FOnlineSessionSearchResult / EOnJoinSessionCompleteResult::Type은 BlueprintType이 아니므로
// FindSessions/JoinSession 결과는 C++ 전용 델리게이트로만 노출한다 (UI 바인딩은 여기서 다루지 않음).
DECLARE_MULTICAST_DELEGATE_TwoParams(FGoHomeSessionOnFindComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
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

	virtual void Deinitialize() override;

	void CreateSession(int32 NumPublicConnections);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGoHomeSessionOnCreateComplete OnCreateSessionComplete_Delegate;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGoHomeSessionOnDestroyComplete OnDestroySessionComplete_Delegate;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGoHomeSessionOnStartComplete OnStartSessionComplete_Delegate;

	FGoHomeSessionOnFindComplete OnFindSessionsComplete_Delegate;
	FGoHomeSessionOnJoinComplete OnJoinSessionComplete_Delegate;

protected:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// 파괴 대기 중 재개설 요청이 들어온 경우 파괴 완료 후 이 인원수로 다시 CreateSession을 호출한다.
	bool bCreateSessionOnDestroy = false;
	int32 LastNumPublicConnections = 4;
};
