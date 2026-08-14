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
	
public:
	UPROPERTY(BlueprintAssignable, category = "Title")
	FGoHomeTitleOnFindComplete OnFindComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Title")
	FGoHomeTitleOnJoinComplete OnJoinComplete;

private:
	TWeakObjectPtr<USessionSubsystem> CachedSessionSubsystem;
	
	// HandleFindComplete에서 받은 원본 결과를 보관 — BP가 "몇 번째 항목 선택"으로 알려주면                   
	// 이 배열의 같은 인덱스를 SessionSubsystem::JoinSession에 넘기기 위함                                    
	TArray<FOnlineSessionSearchResult> CachedSearchResults;
};
