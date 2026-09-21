// 


#include "Core/TitlePlayerController.h"
#include "Core/SessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "CineCameraActor.h"

void ATitlePlayerController::CreateGameSession(int32 NumPublicConnections)
{
	if (CachedSessionSubsystem.IsValid())                                                                     
	{                                                                                                         
		CachedSessionSubsystem->CreateSession(NumPublicConnections);                                      
	}
}

void ATitlePlayerController::FindGameSessions(int32 MaxSearchResults)
{
	if (bFindSessionsInFlight)
	{
		// 이전 검색이 아직 안 끝났음 - 여기서 또 호출하면 SessionSubsystem에 델리게이트가 중첩 등록됨
		return;
	}

	if (CachedSessionSubsystem.IsValid())
	{
		bFindSessionsInFlight = true;
		CachedSessionSubsystem->FindSessions(MaxSearchResults);
	}
}

void ATitlePlayerController::AutoRefreshFindSessions()
{
	FindGameSessions(SessionListMaxSearchResults);
}

void ATitlePlayerController::JoinSessionByIndex(int32 Index)
{
	if (!CachedSessionSubsystem.IsValid() || !CachedSearchResults.IsValidIndex(Index))                        
	{                                                                                                         
		return;                                                                                           
	}                                                                                                         
                                                                                                                  
	CachedSessionSubsystem->JoinSession(CachedSearchResults[Index]);
}

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsLocalController())
	{
		return;
	}
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		CachedSessionSubsystem = GameInstance->GetSubsystem<USessionSubsystem>();              
	}
	
	if (CachedSessionSubsystem.IsValid())
	{
		CachedSessionSubsystem->OnCreateComplete.AddDynamic(this, &ThisClass::HandleCreateComplete);

		CachedSessionSubsystem->OnFindComplete.AddUObject(this, &ThisClass::HandleFindComplete);
		CachedSessionSubsystem->OnJoinComplete.AddUObject(this, &ThisClass::HandleJoinComplete);
	}

	if (ACineCameraActor* Camera = TitleCamera.LoadSynchronous())
	{
		SetViewTargetWithBlend(Camera, 0.f);
	}

	if (IsLocalController() && SessionListAutoRefreshInterval > 0.f)
	{
		// 진입 즉시 1회 조회 + 이후 주기 반복
		AutoRefreshFindSessions();
		GetWorldTimerManager().SetTimer(SessionListAutoRefreshTimerHandle, this, &ThisClass::AutoRefreshFindSessions, SessionListAutoRefreshInterval, true);
	}
}

void ATitlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SessionListAutoRefreshTimerHandle);

	if (CachedSessionSubsystem.IsValid())
	{
		CachedSessionSubsystem->OnCreateComplete.RemoveDynamic(this, &ThisClass::HandleCreateComplete);

		CachedSessionSubsystem->OnFindComplete.RemoveAll(this);
		CachedSessionSubsystem->OnJoinComplete.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ATitlePlayerController::HandleCreateComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)                                                                                      
	{                                                                                                         
		return;                                                                                           
	}                                                                                                         
                                                                                                                  
	// 완료1(로비↔탐사 시멀리스)에서 확인된 패턴과 동일: bAbsolute=true, "?listen" 접미사                     
	GetWorld()->ServerTravel(TEXT("/Game/GoHome/Maps/LV_Lobby?listen"), true);
}

void ATitlePlayerController::HandleFindComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	bFindSessionsInFlight = false;

	CachedSearchResults = SessionResults;
	
	TArray<FString> DisplayNames; 
	for (const FOnlineSessionSearchResult& Result : CachedSearchResults)
	{
		DisplayNames.Add(Result.Session.OwningUserName); 
	}
	
	OnFindComplete.Broadcast(DisplayNames, bWasSuccessful);
}

void ATitlePlayerController::HandleJoinComplete(EOnJoinSessionCompleteResult::Type Result)
{
        bool bSuccess = false;                                                                                    
                                                                                                                  
        if (Result == EOnJoinSessionCompleteResult::Success)                                                      
        {                                                                                                         
                if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())                                        
                {                                                                                                 
                        if (IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface())                
                        {                                                                                         
                                FString ConnectString;                                                            
                                if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))  
                                {                                                                                 
                                        ClientTravel(ConnectString, TRAVEL_Absolute);                             
                                        bSuccess = true;                                                          
                                }                                                                                 
                        }                                                                                         
                }                                                                                                 
        }                                                                                                         
                                                                                                                  
        OnJoinComplete.Broadcast(bSuccess);
}
