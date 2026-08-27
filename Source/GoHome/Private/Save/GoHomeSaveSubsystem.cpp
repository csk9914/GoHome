

#include "Save/GoHomeSaveSubsystem.h"
#include "Save/GoHomeSaveGame.h"
#include "Core/ExpeditionState.h"
#include "Kismet/GameplayStatics.h"
#include "Core/GoHomeGameState.h"
#include "Engine/World.h"

namespace
{
	const FString GoHomeSaveSlotName = TEXT("GoHomeSave");
	constexpr int32 GoHomeSaveUserIndex = 0;
}

void UGoHomeSaveSubsystem::Initialize(FSubsystemCollectionBase& CollectionBase)
{
	Super::Initialize(CollectionBase);
	
	// 디스크에 그 이름의 세이브 파일이 실제로 있는지 확인
	if (UGameplayStatics::DoesSaveGameExist(GoHomeSaveSlotName, GoHomeSaveUserIndex))
	{
		SaveGame = Cast<UGoHomeSaveGame>(UGameplayStatics::LoadGameFromSlot(GoHomeSaveSlotName, GoHomeSaveUserIndex));
		
	}
	
	// 파일이 없거나, 캐스트에 실패했을 경우
	if (!SaveGame)
	{
		// 새로운 빈 SaveGame 인스턴스를 만듬
		SaveGame = Cast<UGoHomeSaveGame>(UGameplayStatics::CreateSaveGameObject(UGoHomeSaveGame::StaticClass()));
	}
	
	// FCoreUObjectDelegates::PostLoadMapWithWorld : 엔진이 맵 로드를 끌낼 때마다 전역으로 쏘는 델리게이트
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGoHomeSaveSubsystem::OnPostLoadMap);
}

void UGoHomeSaveSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	
	Super::Deinitialize();
}

void UGoHomeSaveSubsystem::SaveToDisk()
{
	if (!SaveGame)
	{
		return;
	}
	
	UGameplayStatics::SaveGameToSlot(SaveGame, GoHomeSaveSlotName, GoHomeSaveUserIndex);
}

void UGoHomeSaveSubsystem::OnExpeditionStateChanged(EExpeditionState NewState)
{
	if (NewState == EExpeditionState::Lobby)
	{
		SaveToDisk();
	}
}

void UGoHomeSaveSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld != GetGameInstance()->GetWorld() || LoadedWorld->GetNetMode() == NM_Client)
	{
		return;
	}
	
	AGoHomeGameState* GameState = LoadedWorld->GetGameState<AGoHomeGameState>();
	if (!GameState)
	{
		return;
	}
	
	GameState->OnStateChanged.AddUniqueDynamic(this, &UGoHomeSaveSubsystem::OnExpeditionStateChanged);
	
	if (GameState->GetCurrentState() == EExpeditionState::Lobby)
	{
		SaveToDisk();
	}
}
