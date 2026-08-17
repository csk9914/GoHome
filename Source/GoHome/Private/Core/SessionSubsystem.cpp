

#include "Core/SessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

// 공용 테스트 AppID(480)를 다른 프로젝트들과 같이 쓰기 때문에, 이 키로 우리 세션만 서버 사이드에서 걸러낸다
static const FName GOHOME_SESSION_KEY = TEXT("GOHOME_MATCH");

// 멤버 초기화 리스트를 통해 엔진 비동기 콜백용 내부 델리게이트들을 C++ 함수 바인딩으로 미리 생성
USessionSubsystem::USessionSubsystem() :
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	// 현재 활성화된 OnlineSubsystem(Steam 또는 NULL)을 찾아 SessionInterface를 가져옴
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
}

// GameInstance가 파괴될 때(게임 종료 등) 등록된 모든 델리게이트 핸들을 안전하게 해제하고,
// 남아있는 세션 정리 및 스마트 포인터들을 리셋
void USessionSubsystem::Deinitialize()
{
	if (SessionInterface.IsValid())
	{
		// 메모리 누수 및 중복 호출 방지를 위한 핸들 해제
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);

		// 기존에 생성된 게임 세션이 남아있다면 파괴
		if (SessionInterface->GetNamedSession(NAME_GameSession))
		{
			SessionInterface->DestroySession(NAME_GameSession);
		}
	}

	// 스마트 포인터 수동 해제
	LastSessionSearch.Reset();
	LastSessionSettings.Reset();
	SessionInterface.Reset();
	Super::Deinitialize();
}



// 리슨 서버(Listen Server) 세션을 생성
// 이미 만들어진 세션이 있다면 파괴 완료 후 다시 생성하도록 예약(bCreateSessionOnDestroy)
void USessionSubsystem::CreateSession(int32 NumPublicConnections)
{
	if (!SessionInterface.IsValid())
	{
		OnCreateComplete.Broadcast(false);
		return;
	}

	// 이미 기존 세션이 존재하면 기존 세션을 파괴하고, 파괴 완료 후 재생성하도록 예약
	if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		DestroySession();
		return;
	}

	// 완료 콜백 델리게이트 핸들 등록
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// 세션 세팅 설정 (스팀/LAN 공용 세팅)
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());

	// 서브시스템 이름이 NULL(SubsystemNULL)이면 랜선(LAN) 매치로 설정
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		LastSessionSettings->bIsLANMatch = (Subsystem->GetSubsystemName() == "NULL");
	}

	LastSessionSettings->NumPublicConnections = NumPublicConnections;			// 최대 허용 인원수
	LastSessionSettings->bAllowJoinInProgress = true;							// 게임 진행 중 참가 허용
	LastSessionSettings->bAllowJoinViaPresence = true;							// 스팀 친구 초대/참가 허용 (Presence)
	LastSessionSettings->bShouldAdvertise = true;								// 세션 목록 노출 여부
	LastSessionSettings->bUsesPresence = true;									// Presence 기능 사용
	LastSessionSettings->bUseLobbiesIfAvailable = true;							// 스팀 로비 API(Steam Lobbies) 사용
	// BuildUniqueId는 엔진 기본값(빌드 기준 자동 계산)에 맡김 — 여기서 고정값(예: 1)으로 강제하면
	// FindSessions 쪽 검색 요청의 빌드 ID와 불일치해서 LAN 검색이 결과 0개로 조용히 실패함

	// 공용 AppID(480)를 쓰는 다른 프로젝트들의 로비와 섞이지 않도록 식별 키를 광고
	LastSessionSettings->Set(GOHOME_SESSION_KEY, true, EOnlineDataAdvertisementType::ViaOnlineService);

	// 로컬 플레이어의 NetID를 가져와 세션 생성 요청
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		// 생성 요청 실패 시 등록했던 델리게이트 해제 및 실패 브로드캐스트
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		OnCreateComplete.Broadcast(false);
	}
}

// 현재 네트워크(스팀 로비 또는 LAN)에 개설된 세션들을 검색
void USessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// 검색 조건 옵션 설정
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;														// 최대 검색 결과 수
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";						// LAN 여부

	if (!LastSessionSearch->bIsLanQuery)
	{
		LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);	// 로비 세션 검색 (Steam 전용, LAN 쿼리엔 적용 안 함)

		// 공용 AppID(480)의 다른 프로젝트 로비를 제외하고 우리 세션만 서버 사이드에서 필터링
		LastSessionSearch->QuerySettings.Set(GOHOME_SESSION_KEY, true, EOnlineComparisonOp::Equals);
	}

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		// 검색 요청 실패 시 델리게이트 해제 및 빈 결과 반환
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		OnFindComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

// FindSessions 결과로 얻은 SearchResult 중 특정 세션에 참가를 요청
void USessionSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		OnJoinComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		OnJoinComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

// 생성되었거나 참가한 세션을 삭제/이탈 처리
void USessionSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		OnDestroyComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		OnDestroyComplete.Broadcast(false);
	}
}

// 게임 매치가 실제 시작되었음을 세션에 알림
void USessionSubsystem::StartSession()
{
	if (!SessionInterface.IsValid())
	{
		OnStartComplete.Broadcast(false);
		return;
	}

	StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

	if (!SessionInterface->StartSession(NAME_GameSession))
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
		OnStartComplete.Broadcast(false);
	}
}


// 세션 생성 요청 결과 수신 콜백
void USessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	// 외부(UI, GameInstance 등)에 세션 생성 성공 여부 방송
	OnCreateComplete.Broadcast(bWasSuccessful);
}

// 세션 검색 요청 결과 수신 콜백
void USessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	if (!LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() <= 0 || !bWasSuccessful)
	{
		OnFindComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// 정상 검색 완료: 검색된 세션 목록과 성공(true) 알림 방송
	OnFindComplete.Broadcast(LastSessionSearch->SearchResults, true);
}

// 세션 참가 요청 결과 수신 콜백
void USessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	// 외부(UI/호출측)에 참가 결과 Enum(성공/방가득참/비공개 등) 방송
	OnJoinComplete.Broadcast(Result);
}

// 세션 퇴장 요청 결과 수신 콜백
void USessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections);
	}

	// 외부(UI 등)에 세션 파괴 완료 알림 방송
	OnDestroyComplete.Broadcast(bWasSuccessful);
}

// 세션 진행 시작 요청 결과 수신 콜백
void USessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}

	// 외부(UI 등)에 게임 시작 처리 완료 알림 방송
	OnStartComplete.Broadcast(bWasSuccessful);
}

