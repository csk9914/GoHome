// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "TitlePlayerController.generated.h"

// 세션 검색 결과 브로드캐스트용 (BP가 다루기 쉬운 형태로 가공해서 넘김)                 
// FOnlineSessionSearchResult는 UPROPERTY 불가 → 표시용 이름 배열 + 성공 여부만 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGoHomeTitleOnFindComplete, const TArray<FString>&, SessionDisPlayNames, bool, bWasSuccessful);

// 세션 참가 시도 결과 브로드캐스트용 (성공/실패만 — 실패 사유가 필요해지면 그때 enum 추가)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoHomeTitleOnJoinComplete, bool, bWasSuccessful);


class USessionSubsystem;
class ACineCameraActor;

/**
 *
 */
UCLASS()
class GOHOME_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Title")
	void CreateGameSession(int32 NumPublicConnections);
                                                                                                                  
	UFUNCTION(BlueprintCallable, Category = "Title")                                                          
	void FindGameSessions(int32 MaxSearchResults);
	
	// WBP_Title에서 "n번째 검색 결과 참가" 버튼 클릭 시 호출                                                 
	UFUNCTION(BlueprintCallable, Category = "Title")                                                          
	void JoinSessionByIndex(int32 Index);
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleCreateComplete(bool bWasSuccessful);

	void HandleFindComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void HandleJoinComplete(EOnJoinSessionCompleteResult::Type Result);

	// 자동 새로고침 타이머가 호출하는 진입점 (SessionListMaxSearchResults 고정값 사용)
	void AutoRefreshFindSessions();

public:
	UPROPERTY(BlueprintAssignable, category = "Title")
	FGoHomeTitleOnFindComplete OnFindComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Title")
	FGoHomeTitleOnJoinComplete OnJoinComplete;

	// CineCameraActor는 AutoActivateForPlayer 카테고리를 숨기므로 여기서 명시적으로 뷰타겟을 지정
	UPROPERTY(EditAnywhere, Category = "Title")
	TSoftObjectPtr<ACineCameraActor> TitleCamera;

	// 타이틀 화면 진입 시 자동 새로고침 주기(초). BeginPlay에서 즉시 1회 + 이 주기로 반복 조회.
	UPROPERTY(EditAnywhere, Category = "Title")
	float SessionListAutoRefreshInterval = 5.f;

	// 자동 새로고침이 사용하는 MaxSearchResults (수동 "방 찾기" 버튼은 BP에서 별도 값을 넘김)
	UPROPERTY(EditAnywhere, Category = "Title")
	int32 SessionListMaxSearchResults = 50;

private:
	TWeakObjectPtr<USessionSubsystem> CachedSessionSubsystem;

	// HandleFindComplete에서 받은 원본 결과를 보관 — BP가 "몇 번째 항목 선택"으로 알려주면
	// 이 배열의 같은 인덱스를 SessionSubsystem::JoinSession에 넘기기 위함
	TArray<FOnlineSessionSearchResult> CachedSearchResults;

	FTimerHandle SessionListAutoRefreshTimerHandle;

	// FindGameSessions 진행 중 타이머/수동클릭이 겹쳐 SessionSubsystem에 델리게이트가 중첩 등록되는 것을 방지
	bool bFindSessionsInFlight = false;
};
