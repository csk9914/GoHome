// 


#include "Core/VoiceChatSubsystem.h"

#include "OnlineSubsystemUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "AI/NoiseType.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
// 마이크 없이 음성 HUD 확인용: GoHome.Voice.FakeTalk <PlayerArray 인덱스> <0|1>
static FAutoConsoleCommandWithWorldAndArgs GFakeTalkCommand(
	TEXT("GoHome.Voice.FakeTalk"),
	TEXT("음성 말하기 상태를 흉내 낸다(로컬 브로드캐스트만). 인자: <PlayerArray 인덱스> <0|1>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UVoiceChatSubsystem* Voice = GameInstance ? GameInstance->GetSubsystem<UVoiceChatSubsystem>() : nullptr;
		const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		const bool bTalking = Args.Num() > 1 ? FCString::Atoi(*Args[1]) != 0 : true;
		if (Voice && GameState && GameState->PlayerArray.IsValidIndex(Index))
		{
			Voice->OnTalkingStateChanged.Broadcast(GameState->PlayerArray[Index], bTalking);
		}
	}));
#endif

void UVoiceChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface(GetWorld()))
	{
		VoiceInterface->OnPlayerTalkingStateChangedDelegates.AddUObject(this, &UVoiceChatSubsystem::HandlePlayerTalkingStateChanged);
	}
	
	GetWorld()->GetTimerManager().SetTimer(ProximityMuteTimerHandle, this, &UVoiceChatSubsystem::UpdateProximityMute, 0.5f, true);
}

void UVoiceChatSubsystem::Deinitialize()
{
	GetWorld()->GetTimerManager().ClearTimer(ProximityMuteTimerHandle);
	
	if (IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface(GetWorld()))
	{
		VoiceInterface->OnPlayerTalkingStateChangedDelegates.RemoveAll(this);
	}
	
	Super::Deinitialize();
}

void UVoiceChatSubsystem::HandlePlayerTalkingStateChanged(FUniqueNetIdRef NetId, bool bIsTalking)
{
	AGameStateBase* GameStateBase = GetWorld()->GetGameState();
	if (!GameStateBase)
	{
		return;
	}
	
	// 엔진 델리게이트가 넘겨주는 건 FUniqueNetIdRef 뿐이라,
	// APlayerState로 변환한 뒤 다시 쏘는 구조
	const FUniqueNetIdRepl TargetId(NetId);
	for (APlayerState* PlayerState : GameStateBase->PlayerArray)
	{
		if (PlayerState && PlayerState->GetUniqueId() == TargetId)
		{
			OnTalkingStateChanged.Broadcast(PlayerState, bIsTalking);
			
			// 말하기 상태일 때만 소음 발생 - 말을 멈추면 노이즈 트리거 안 함
			// GenerateNoise는 1회성 동기 함수라 시작 시점 1회면 충분
			if (bIsTalking && GetWorld()->GetNetMode() != NM_Client)
			{
				// 몬스터가 추적 가능하게 실제 월드 상에 배치된 Pawn을 넘김
				if (APawn* Pawn = PlayerState->GetPawn())
				{
					UGoHomeNoiseLibrary::GenerateNoise(this, Pawn->GetActorLocation(), 1000.f, ENoiseType::Medium, Pawn);
				}
			}
			break;
		}
	}
}

void UVoiceChatSubsystem::UpdateProximityMute()
{
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}
	
	AGameStateBase* GameStateBase = GetWorld()->GetGameState();
	if (!GameStateBase)
	{
		return;
	}
	
	const TArray<APlayerState*> PlayerStates = GameStateBase->PlayerArray;
	for (int32 i = 0; i < PlayerStates.Num(); i++)
	{
		APlayerState* StateA = PlayerStates[i];
		APawn* PawnA = StateA ? StateA->GetPawn() : nullptr;
		APlayerController* ControllerA = StateA ? StateA->GetPlayerController() : nullptr;

		for (int32 j = i + 1; j < PlayerStates.Num(); j++)
		{
			APlayerState* StateB = PlayerStates[j];
			APawn* PawnB = StateB ? StateB->GetPawn() : nullptr;
			APlayerController* ControllerB = StateB ? StateB->GetPlayerController() : nullptr;

			if (!PawnA || !PawnB || !ControllerA || !ControllerB)
			{
				continue;
			}

			const bool bInRange = FVector::DistSquared(PawnA->GetActorLocation(), PawnB->GetActorLocation()) <= FMath::Square(ProximityRange);

			if (bInRange)
			{
				ControllerA->ServerUnmutePlayer(StateB->GetUniqueId());
				ControllerB->ServerUnmutePlayer(StateA->GetUniqueId());
			}
			else
			{
				ControllerA->ServerMutePlayer(StateB->GetUniqueId());
				ControllerB->ServerMutePlayer(StateA->GetUniqueId());
			}
		}
	}
}
