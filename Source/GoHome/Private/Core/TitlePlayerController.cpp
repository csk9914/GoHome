// 


#include "Core/TitlePlayerController.h"
#include "Core/SessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"                                                      
#include "Online/OnlineSessionNames.h"

void ATitlePlayerController::CreateGameSession(int32 NumPublicConnections)
{
	if (CachedSessionSubsystem.IsValid())                                                                     
	{                                                                                                         
		CachedSessionSubsystem->CreateSession(NumPublicConnections);                                      
	}
}

void ATitlePlayerController::FindGameSessions(int32 MaxSearchResults)
{
	if (CachedSessionSubsystem.IsValid())                                                                     
	{                                                                                                         
		CachedSessionSubsystem->FindSessions(MaxSearchResults);                                           
	}
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
}

void ATitlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
