// 

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VoiceChatSubsystem.generated.h"

// 말하기 상태 변경 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGoHomeVoiceOnTalkingStateChanged, APlayerState*, Speaker, bool, bIsTalking);

/**
 * IOnlineVoice의 말하기 상태를 구독해 BP로 브로드캐스트하고,
 * 서버쪽에서는 몬스터 소음 감지로 연결한다.
 */

UCLASS()
class GOHOME_API UVoiceChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UPROPERTY(BlueprintAssignable, Category="Voice")
	FGoHomeVoiceOnTalkingStateChanged OnTalkingStateChanged;
	
private:
	// 왜 APlayerState*가 아니라 FUniqueNetIdRef 인가:
	// 엔진 델리게이트가 넘겨주는 건 FUniqueNetIdRef 뿐이라,
	// 내부 핸들러에서 이걸 받아 APlayerState로 변환한 뒤 다시 쏘는 구조
	void HandlePlayerTalkingStateChanged(FUniqueNetIdRef NetId, bool bIsTalking);
	
	void UpdateProximityMute();
	
	
	FTimerHandle ProximityMuteTimerHandle;
	float ProximityRange = 3000.f;
	
};
