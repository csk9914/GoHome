// 


#include "Core/LobbyGameState.h"
#include "Core/ExpeditionZoneDataAsset.h"
#include "Net/UnrealNetwork.h"

ALobbyGameState::ALobbyGameState()
{
	// 생성자에서 대입하는 건 아직 리플리케이션이 시작되기 전이라 직접 대입
	// 어차피 이 시점엔 델리게이트 구독자(위젯 등)가 아직 없어서 브로드캐스트할 대상도 없음
	CurrentState = EExpeditionState::Lobby;
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// SelectedZoneId 변수를 클라이언트들에게 동기화(Replicate)하도록 등록
	DOREPLIFETIME(ALobbyGameState, SelectedZoneId);
}

void ALobbyGameState::SetSelectedZone(FName ZoneId)
{
	UE_LOG(LogTemp, Warning, TEXT("SetSelectedZone called with %s"), *ZoneId.ToString());
	
	// 서버가 클라이언트 RPC로 받은 값을 그대로 신뢰하지 않고 검증
	const bool bIsValidZone = AvailableZones.ContainsByPredicate
	([ZoneId](const UExpeditionZoneDataAsset* Zone)
	{
		return Zone && (Zone ->ZoneId == ZoneId);
	});
	
	if (!bIsValidZone) return;

	SelectedZoneId = ZoneId;

	// OnRep은 서버 자신에게는 자동 호출되지 않으므로 직접 호출해 서버(호스트) 쪽 UI도 갱신되게 함
	OnRep_SelectedZone();
}

const UExpeditionZoneDataAsset* ALobbyGameState::GetSelectedZone() const
{
	const TObjectPtr<UExpeditionZoneDataAsset>* Found = AvailableZones.FindByPredicate
	([this](const UExpeditionZoneDataAsset* Zone)
	{
		return Zone &&(Zone->ZoneId == SelectedZoneId);
	});
	
	return Found ? *Found : nullptr;
}

void ALobbyGameState::OnRep_SelectedZone()
{
	OnSelectedZoneChanged.Broadcast(SelectedZoneId);
}
