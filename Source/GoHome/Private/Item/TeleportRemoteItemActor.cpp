


#include "Item/TeleportRemoteItemActor.h"

#include "Item/TeleportStationActor.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Interaction/InventoryComponent.h"

void ATeleportRemoteItemActor::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn || !bIsActiveHeld || !CanUse()) return;

	ATeleportStationActor* Station = FindStation();
	if (!Station) return;

	ChannelEndServerTime = GetNowServerTime() + HoldDuration;

	GetWorldTimerManager().SetTimer(ChannelTimerHandle, this, &ATeleportRemoteItemActor::OnChannelComplete, HoldDuration, false);

	Station->SetCharging(true);

	OnRep_ChannelEndServerTime(); // 서버 자신에게는 RepNotify가 안 뜨므로 직접 호출
}

void ATeleportRemoteItemActor::ServerCancelSpecialAction()
{
	if (!HasAuthority() || !IsChanneling()) return;

	ClearChannel();
}

bool ATeleportRemoteItemActor::CanUse() const
{
	return !bConsumed && !IsChanneling();
}

float ATeleportRemoteItemActor::GetChannelProgress() const
{
	if (!IsChanneling() || HoldDuration <= 0.f) return 0.f;

	const float Remaining = ChannelEndServerTime - GetNowServerTime();
	return FMath::Clamp(1.f - (Remaining / HoldDuration), 0.f, 1.f);
}

float ATeleportRemoteItemActor::GetChannelRemainingSeconds() const
{
	if (!IsChanneling()) return 0.f;

	return FMath::Max(0.f, ChannelEndServerTime - GetNowServerTime());
}

void ATeleportRemoteItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATeleportRemoteItemActor, ChannelEndServerTime);
	DOREPLIFETIME(ATeleportRemoteItemActor, bConsumed);
}

void ATeleportRemoteItemActor::ClearChannel()
{
	GetWorldTimerManager().ClearTimer(ChannelTimerHandle);

	ChannelEndServerTime = 0.f;

	if (CachedStation)
	{
		CachedStation->SetCharging(false);
	}

	OnRep_ChannelEndServerTime();
}

ATeleportStationActor* ATeleportRemoteItemActor::FindStation()
{
	if (CachedStation) return CachedStation;

	for (TActorIterator<ATeleportStationActor> Iter(GetWorld()); Iter; ++Iter)
	{
		CachedStation = *Iter;
		break;
	}

	if (!CachedStation)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TeleportRemote] 레벨에 ATeleportStaionActor가 배치되지 않았습니다."));
	}

	return CachedStation;
}

void ATeleportRemoteItemActor::OnRep_ChannelEndServerTime()
{
	const bool bNowChanneling = IsChanneling();
	if (bNowChanneling == bCosmeticActive) return;

	bCosmeticActive = bNowChanneling;

	const bool bLocalUser = IsLocallyHeld();

	if (bNowChanneling)
	{
		OnChannelStartedCosmetic(bLocalUser);
	}
	else
	{
		OnChannelEndedCosmetic(bLocalUser);
	}

	OnChannelChanged.Broadcast(bNowChanneling);
}

void ATeleportRemoteItemActor::OnChannelComplete()
{
	// 타이머가 도는 사이 상태가 깨졌을 수 있으므로 재검증
	if (!HasAuthority() || !HoldingPawn || !bIsActiveHeld)
	{
		ClearChannel();
		return;
	}

	ATeleportStationActor* Station = FindStation();
	if (!Station)
	{
		ClearChannel();
		return;
	}

	const FTransform Target = Station->GetTeleportTransform();

	if (ACharacter* Character = Cast<ACharacter>(HoldingPawn))
	{
		// 몸 회전은 유지하고 위치만 옮긴 뒤 시선만 따로 맞춤
		Character->TeleportTo(Target.GetLocation(), Character->GetActorRotation());

		// 서버에서 SetControlRotation을 해도 클라 시선에는 반영되지 않음 -> 클라 RPC로 보정
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			PlayerController->ClientSetRotation(Target.GetRotation().Rotator(), false);
		}
	}

	bConsumed = true;

	Station->Multicast_PlayArrivalBurst();

	ClearChannel();

	// 1회용 - 즉시 소멸
	ConsumeSelf();
}

void ATeleportRemoteItemActor::UpdateAttachment(APawn* OldHoldingPawn)
{
	Super::UpdateAttachment(OldHoldingPawn);

	if (HasAuthority() && IsChanneling() && (!HoldingPawn || !bIsActiveHeld))
	{
		ClearChannel();
	}
}

float ATeleportRemoteItemActor::GetNowServerTime() const
{
	const UWorld* World = GetWorld();
	if (!World) return 0.f;
	// 클라에서도 서버 시각 근사치를 준다. 서버에서는 서버 월드 시각 그대로
	if (const AGameStateBase* GameState = World->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return World->GetTimeSeconds();
}

bool ATeleportRemoteItemActor::IsLocallyHeld() const
{
	const APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	if (!LocalPC) return false;

	return LocalPC->GetPawn() == HoldingPawn;
}

void ATeleportRemoteItemActor::ConsumeSelf()
{
	if (!HasAuthority()) return;

	if (HoldingPawn)
	{
		if (UInventoryComponent* Inventory = HoldingPawn->FindComponentByClass<UInventoryComponent>())
		{
			// 슬롯 비우기 + 캐릭터 홀드 상태 재계산까지 처리
			Inventory->RemoveItem(this);
		}
	}

	Destroy();
}
