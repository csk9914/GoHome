// 


#include "Core/Actors/ZoneSelectMonitor.h"
#include "Core/GoHomePlayerController.h"
#include "GameFramework/Pawn.h"
#include "Core/LobbyGameState.h"

AZoneSelectMonitor::AZoneSelectMonitor()
{
}

bool AZoneSelectMonitor::CanInteract(APawn* InstigatorPawn) const
{
	// 현재 로비 맵일 경우에만 활성화
	return GetWorld()->GetGameState<ALobbyGameState>() != nullptr;
}

void AZoneSelectMonitor::OnInteract(APawn* InstigatorPawn)
{
	// OnInteract는 서버에서 실행 (InteractionComponent::Server_RequestInteract_Implementation 경유)
	// InstigatorPawn->GetController()로 그 상호작용을 한 플레이어의 컨트롤러를 얻은 다음
	// Client_OpenSelectZone()(Client RPC)을 호출해서 그 플레이어에게만 UI를 열라고 알림
	
	if (AGoHomePlayerController* PlayerController = Cast<AGoHomePlayerController>(InstigatorPawn->GetController()))
	{
		PlayerController->Client_OpenSelectZone();
	}
}

