

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "SessionTestActor.generated.h"

class USessionSubsystem;

/**
 * USessionSubsystem 스모크 테스트용 임시 액터. 레벨에 배치 후 Play, 디테일 패널의
 * CallInEditor 버튼(Test Create/Find/Join/Destroy Session)을 눌러 세션 흐름을 확인한다.
 * 정식 로비 UI(UI/)가 만들어지면 삭제할 것 — 정식 클래스가 아니므로 ARCHITECTURE.md에 등록하지 않는다.
 */
UCLASS()
class GOHOME_API ASessionTestActor : public AActor
{
	GENERATED_BODY()

public:
	ASessionTestActor();

	UPROPERTY(EditAnywhere, Category = "Session Test")
	int32 NumPublicConnections = 4;

	UPROPERTY(EditAnywhere, Category = "Session Test")
	int32 MaxSearchResults = 50;

	UFUNCTION(CallInEditor, Category = "Session Test")
	void TestCreateSession();

	UFUNCTION(CallInEditor, Category = "Session Test")
	void TestFindSessions();

	UFUNCTION(CallInEditor, Category = "Session Test")
	void TestJoinFirstResult();

	UFUNCTION(CallInEditor, Category = "Session Test")
	void TestDestroySession();

protected:
	virtual void BeginPlay() override;

private:
	USessionSubsystem* GetSessionSubsystem() const;

	UFUNCTION()
	void HandleCreateSessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleDestroySessionComplete(bool bWasSuccessful);

	void HandleFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);

	static void PrintResult(const FString& Message, bool bSuccess);

	TArray<FOnlineSessionSearchResult> LastFoundSessions;
};
