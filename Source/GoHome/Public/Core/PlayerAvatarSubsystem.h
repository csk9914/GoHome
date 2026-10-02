#pragma once

#include "CoreMinimal.h"
#include "GameFramework/OnlineReplStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerAvatarSubsystem.generated.h"

class UTexture2D;

// 아바타 텍스처가 새로 준비됐을 때(키 = UniqueNetId 문자열)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerAvatarReady, const FString& /*NetIdKey*/);

/**
 * 플레이어 프로필 아바타(Steam 중간 크기 64x64)를 로컬에서 받아 텍스처로 캐시한다.
 * OSS v1 에 아바타 API 가 없어 Steamworks(ISteamFriends/ISteamUtils)를 직접 호출한다.
 * Steam 이 아닌 환경(에디터 PIE·Null OSS)이나 아바타가 없는 계정은 nullptr — 위젯은 이니셜로 대체한다.
 * 로컬 표시 전용이라 복제 상태 없음.
 */
UCLASS()
class GOHOME_API UPlayerAvatarSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// 캐시돼 있으면 바로 반환. 없으면 nullptr 를 주고 백그라운드로 요청 — 준비되면 OnAvatarReady.
	UTexture2D* GetAvatar(const FUniqueNetIdRepl& NetId);

	static FString MakeKey(const FUniqueNetIdRepl& NetId);

	FOnPlayerAvatarReady OnAvatarReady;

private:
	void PollPending();

	// 성공(텍스처 생성) 또는 확정 실패면 true — 대기 목록에서 뺀다
	bool TryLoad(const FString& Key, uint64 SteamId);

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTexture2D>> Avatars;

	struct FPendingAvatar
	{
		uint64 SteamId = 0;
		int32 Attempts = 0;
	};
	TMap<FString, FPendingAvatar> Pending;

	// 아바타가 아예 없거나 Steam 계정이 아닌 키 — 다시 요청하지 않는다
	TSet<FString> Unavailable;

	FTimerHandle PollTimerHandle;

	// 페르소나 정보가 늦게 오는 경우(친구 아님 등) 0.5초 간격으로 이만큼 재시도
	static constexpr int32 MaxAttempts = 20;
};
