

#include "Core/SessionTestActor.h"
#include "Core/SessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/Engine.h"

ASessionTestActor::ASessionTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASessionTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (USessionSubsystem* Session = GetSessionSubsystem())
	{
		Session->OnCreateSessionComplete_Delegate.AddDynamic(this, &ASessionTestActor::HandleCreateSessionComplete);
		Session->OnDestroySessionComplete_Delegate.AddDynamic(this, &ASessionTestActor::HandleDestroySessionComplete);
		Session->OnFindSessionsComplete_Delegate.AddUObject(this, &ASessionTestActor::HandleFindSessionsComplete);
		Session->OnJoinSessionComplete_Delegate.AddUObject(this, &ASessionTestActor::HandleJoinSessionComplete);
	}
}

USessionSubsystem* ASessionTestActor::GetSessionSubsystem() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<USessionSubsystem>();
		}
	}
	return nullptr;
}

void ASessionTestActor::TestCreateSession()
{
	if (USessionSubsystem* Session = GetSessionSubsystem())
	{
		PrintResult(TEXT("CreateSession 요청..."), true);
		Session->CreateSession(NumPublicConnections);
	}
}

void ASessionTestActor::TestFindSessions()
{
	if (USessionSubsystem* Session = GetSessionSubsystem())
	{
		PrintResult(TEXT("FindSessions 요청..."), true);
		Session->FindSessions(MaxSearchResults);
	}
}

void ASessionTestActor::TestJoinFirstResult()
{
	if (LastFoundSessions.Num() == 0)
	{
		PrintResult(TEXT("검색된 세션이 없습니다. 먼저 Test Find Sessions를 실행하세요."), false);
		return;
	}

	if (USessionSubsystem* Session = GetSessionSubsystem())
	{
		Session->JoinSession(LastFoundSessions[0]);
	}
}

void ASessionTestActor::TestDestroySession()
{
	if (USessionSubsystem* Session = GetSessionSubsystem())
	{
		Session->DestroySession();
	}
}

void ASessionTestActor::HandleCreateSessionComplete(bool bWasSuccessful)
{
	PrintResult(TEXT("CreateSession 완료"), bWasSuccessful);

	// 스모크 테스트 편의상 호스트를 바로 리슨 서버로 트래블시킨다.
	// 정식 흐름에서는 UI/GameInstance가 이 시점을 결정한다.
	if (bWasSuccessful)
	{
		GetWorld()->ServerTravel(TEXT("?listen"));
	}
}

void ASessionTestActor::HandleDestroySessionComplete(bool bWasSuccessful)
{
	PrintResult(TEXT("DestroySession 완료"), bWasSuccessful);
}

void ASessionTestActor::HandleFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	LastFoundSessions = SessionResults;
	PrintResult(FString::Printf(TEXT("FindSessions 완료 (%d개 발견)"), SessionResults.Num()), bWasSuccessful);
}

void ASessionTestActor::HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	const bool bSuccess = Result == EOnJoinSessionCompleteResult::Success;
	PrintResult(FString::Printf(TEXT("JoinSession 완료 (Result=%d)"), static_cast<int32>(Result)), bSuccess);

	if (!bSuccess)
	{
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid())
	{
		return;
	}

	FString ConnectString;
	if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
}

void ASessionTestActor::PrintResult(const FString& Message, bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("[SessionTestActor] %s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, bSuccess ? FColor::Green : FColor::Red, Message);
	}
}
