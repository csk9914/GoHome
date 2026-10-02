#include "Core/PlayerAvatarSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

void UPlayerAvatarSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().ClearTimer(PollTimerHandle);
	}
	Pending.Reset();
	Super::Deinitialize();
}

FString UPlayerAvatarSubsystem::MakeKey(const FUniqueNetIdRepl& NetId)
{
	return NetId.IsValid() ? NetId->ToString() : FString();
}

UTexture2D* UPlayerAvatarSubsystem::GetAvatar(const FUniqueNetIdRepl& NetId)
{
	const FString Key = MakeKey(NetId);
	if (Key.IsEmpty())
	{
		return nullptr;
	}

	if (const TObjectPtr<UTexture2D>* Found = Avatars.Find(Key))
	{
		return *Found;
	}

	if (Unavailable.Contains(Key) || Pending.Contains(Key))
	{
		return nullptr;
	}

	// Steam 계정 id 는 64비트 SteamID 의 10진 문자열
	static const FName SteamType(TEXT("STEAM"));
	const uint64 SteamId = NetId->GetType() == SteamType ? FCString::Strtoui64(*Key, nullptr, 10) : 0;
	if (SteamId == 0 || !SteamFriends() || !SteamUtils())
	{
		Unavailable.Add(Key);
		return nullptr;
	}

	// 친구가 아닌 플레이어는 페르소나(아바타 포함)를 먼저 요청해야 한다 — false = 이름+아바타
	SteamFriends()->RequestUserInformation(CSteamID(SteamId), false);
	if (TryLoad(Key, SteamId))
	{
		const TObjectPtr<UTexture2D>* Loaded = Avatars.Find(Key);
		return Loaded ? Loaded->Get() : nullptr;
	}

	Pending.Add(Key, FPendingAvatar{ SteamId, 1 });
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		FTimerManager& TimerManager = GameInstance->GetTimerManager();
		if (!TimerManager.IsTimerActive(PollTimerHandle))
		{
			TimerManager.SetTimer(PollTimerHandle, this, &UPlayerAvatarSubsystem::PollPending, 0.5f, true);
		}
	}
	return nullptr;
}

void UPlayerAvatarSubsystem::PollPending()
{
	TArray<FString> Ready;
	TArray<FString> Done;
	for (TPair<FString, FPendingAvatar>& Pair : Pending)
	{
		if (TryLoad(Pair.Key, Pair.Value.SteamId))
		{
			Done.Add(Pair.Key);
			if (Avatars.Contains(Pair.Key))
			{
				Ready.Add(Pair.Key);
			}
		}
		else if (++Pair.Value.Attempts >= MaxAttempts)
		{
			Unavailable.Add(Pair.Key);
			Done.Add(Pair.Key);
		}
	}

	for (const FString& Key : Done)
	{
		Pending.Remove(Key);
	}

	if (Pending.IsEmpty())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			GameInstance->GetTimerManager().ClearTimer(PollTimerHandle);
		}
	}

	for (const FString& Key : Ready)
	{
		OnAvatarReady.Broadcast(Key);
	}
}

bool UPlayerAvatarSubsystem::TryLoad(const FString& Key, uint64 SteamId)
{
	ISteamFriends* Friends = SteamFriends();
	ISteamUtils* Utils = SteamUtils();
	if (!Friends || !Utils)
	{
		Unavailable.Add(Key);
		return true;
	}

	// -1 = 아직 내려받는 중, 0 = 페르소나 미수신이거나 아바타 없음 — 둘 다 재시도(MaxAttempts 후 포기)
	const int32 ImageHandle = Friends->GetMediumFriendAvatar(CSteamID(SteamId));
	if (ImageHandle <= 0)
	{
		return false;
	}

	uint32 Width = 0;
	uint32 Height = 0;
	if (!Utils->GetImageSize(ImageHandle, &Width, &Height) || Width == 0 || Height == 0)
	{
		return false;
	}

	TArray<uint8> Pixels;
	Pixels.SetNumUninitialized(Width * Height * 4);
	if (!Utils->GetImageRGBA(ImageHandle, Pixels.GetData(), Pixels.Num()))
	{
		return false;
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		Unavailable.Add(Key);
		return true;
	}

	Texture->SRGB = true;
	Texture->Filter = TF_Bilinear;
	void* MipData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(MipData, Pixels.GetData(), Pixels.Num());
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();

	Avatars.Add(Key, Texture);
	return true;
}
