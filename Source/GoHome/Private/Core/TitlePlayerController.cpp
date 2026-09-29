// 


#include "Core/TitlePlayerController.h"
#include "Core/SessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "CineCameraActor.h"
#include "Save/GoHomeSaveSubsystem.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogTitleFlow, Log, All);

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
		SetSearchStatus(ETitleSearchStatus::Searching);
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

	if (TitleScreenClass)
	{
		TitleScreen = CreateWidget<UUserWidget>(this, TitleScreenClass);
		if (TitleScreen)
		{
			TitleScreen->AddToViewport();

			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(TitleScreen->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
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

	if (TitleScreen)
	{
		TitleScreen->RemoveFromParent();
		TitleScreen = nullptr;
	}

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
	const TOptional<ETitleHostMode> HostMode = PendingHostMode;
	PendingHostMode.Reset();

	bool bSuccess = bWasSuccessful;

	// 새 원정은 세션 생성이 성공한 뒤에만 초기화한다 — 생성 실패로 기존 진행이 날아가지 않게.
	if (bSuccess && HostMode.IsSet() && HostMode.GetValue() == ETitleHostMode::NewExpedition)
	{
		UGoHomeSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
		bSuccess = SaveSubsystem && SaveSubsystem->StartNewExpedition();

		if (!bSuccess && CachedSessionSubsystem.IsValid())
		{
			CachedSessionSubsystem->DestroySession();
		}
	}

	UE_LOG(LogTitleFlow, Log, TEXT("Host complete: success=%d mode=%d"), bSuccess, HostMode.IsSet() ? static_cast<int32>(HostMode.GetValue()) : -1);
	TitleEvents.OnHostComplete.Broadcast(bSuccess);

	if (!bSuccess)
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
	FTitleSearchSnapshot NewSnapshot;
	NewSnapshot.Generation = SearchSnapshot.Generation + 1;

	for (int32 Index = 0; Index < CachedSearchResults.Num(); ++Index)
	{
		const FString& HostName = CachedSearchResults[Index].Session.OwningUserName;
		DisplayNames.Add(HostName);

		FTitleSessionListing Listing;
		Listing.HostName = FText::FromString(HostName);
		Listing.SearchIndex = Index;
		NewSnapshot.Listings.Add(MoveTemp(Listing));
	}

	if (!bWasSuccessful)
	{
		NewSnapshot.Status = ETitleSearchStatus::Failed;
	}
	else
	{
		NewSnapshot.Status = NewSnapshot.Listings.IsEmpty() ? ETitleSearchStatus::Empty : ETitleSearchStatus::Ready;
	}

	SearchSnapshot = MoveTemp(NewSnapshot);
	UE_LOG(LogTitleFlow, Log, TEXT("Search complete: status=%d count=%d generation=%d"), static_cast<int32>(SearchSnapshot.Status), SearchSnapshot.Listings.Num(), SearchSnapshot.Generation);

	OnFindComplete.Broadcast(DisplayNames, bWasSuccessful);
	TitleEvents.OnSearchUpdated.Broadcast(SearchSnapshot);
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
                                                                                                                  
        bJoinInFlight = false;
        UE_LOG(LogTitleFlow, Log, TEXT("Join complete: success=%d"), bSuccess);

        OnJoinComplete.Broadcast(bSuccess);
        TitleEvents.OnJoinComplete.Broadcast(bSuccess);
}

bool ATitlePlayerController::HasResumableProgress() const
{
	const UGoHomeSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	return SaveSubsystem && SaveSubsystem->HasResumableProgress();
}

FExpeditionProgress ATitlePlayerController::GetProgressSummary() const
{
	const UGoHomeSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	return SaveSubsystem ? SaveSubsystem->BuildProgress() : FExpeditionProgress();
}

void ATitlePlayerController::RequestHost(ETitleHostMode Mode)
{
	if (PendingHostMode.IsSet() || !CachedSessionSubsystem.IsValid())
	{
		return;
	}

	PendingHostMode = Mode;
	UE_LOG(LogTitleFlow, Log, TEXT("Host requested: mode=%d players=%d"), static_cast<int32>(Mode), MaxPlayers);
	CachedSessionSubsystem->CreateSession(MaxPlayers);
}

void ATitlePlayerController::RequestRefresh()
{
	FindGameSessions(SessionListMaxSearchResults);
}

bool ATitlePlayerController::RequestJoin(int32 Generation, int32 SearchIndex)
{
	if (bJoinInFlight || Generation != SearchSnapshot.Generation || !CachedSearchResults.IsValidIndex(SearchIndex) || !CachedSessionSubsystem.IsValid())
	{
		return false;
	}

	bJoinInFlight = true;
	UE_LOG(LogTitleFlow, Log, TEXT("Join requested: generation=%d index=%d"), Generation, SearchIndex);
	CachedSessionSubsystem->JoinSession(CachedSearchResults[SearchIndex]);
	return true;
}

void ATitlePlayerController::SetSearchStatus(ETitleSearchStatus NewStatus)
{
	if (SearchSnapshot.Status == NewStatus)
	{
		return;
	}

	SearchSnapshot.Status = NewStatus;
	TitleEvents.OnSearchUpdated.Broadcast(SearchSnapshot);
}

UGoHomeSaveSubsystem* ATitlePlayerController::GetSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UGoHomeSaveSubsystem>() : nullptr;
}
