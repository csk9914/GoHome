// 


#include "Core/Actors/DepartureButton.h"

#include "Core/ExpeditionZoneDataAsset.h"
#include "Core/GoHomeGameMode.h"
#include "Core/LobbyGameState.h"
#include "Core/DockingDoorComponent.h"
#include "TimerManager.h"


ADepartureButton::ADepartureButton()
{
}

bool ADepartureButton::CanInteract(APawn* InstigatorPawn) const
{
	return true;
}

void ADepartureButton::OnInteract(APawn* InstigatorPawn)
{
	AGoHomeGameMode* GoHomeGameMode = GetWorld()->GetAuthGameMode<AGoHomeGameMode>();
	if (!GoHomeGameMode) return;



	//  현재 로비 맵일 때
	if (ALobbyGameState* LobbyGameState = GetWorld()->GetGameState<ALobbyGameState>())
	{
		// 탐사 맵으롷 이동한다.
		const UExpeditionZoneDataAsset* SelectedZone = LobbyGameState->GetSelectedZone();
		if (!SelectedZone) return;

		GoHomeGameMode->ServerTravelViaLoadingScreen(SelectedZone->MapPath.ToSoftObjectPath().GetLongPackageName());
	}
	
	// 현재 탐사 맵일 때
	else if (AGoHomeGameState* GoHomeGameState = GetWorld()->GetGameState<AGoHomeGameState>())
	{
		// 문을 닫고
		if (UDockingDoorComponent* DoorComponent = GoHomeGameState->GetDockingDoorComponent())
		{
			DoorComponent->SetOpen(false);
		}
		
		// 타이머를 이용해서 문이 닫히는 시간까지 딜레이를 준다
		// DoorCloseDelay 기다린 다음에 로비 맵으로 이동한다.
		GetWorldTimerManager().SetTimer(
			TravelDelayTimer, 
			FTimerDelegate::CreateUObject(this, &ADepartureButton::ExecuteDelayedTravel, GoHomeGameMode->GetLobbyMapPath()),
			DoorCloseDelay,
			false);
		
		// CreateUObject를 쓰는 이유는 
		// this(ADepartureButton 인스턴스)가 콜백 실행 전에 파괴돼도 
		// 엔진이 안전하게 무시하기 때문(레벨 트래블 중 액터가 사라지는 경우)
	}
}

void ADepartureButton::ExecuteDelayedTravel(FString TravelPath)
{
	if (AGoHomeGameMode* GoHomeGameMode = GetWorld()->GetAuthGameMode<AGoHomeGameMode>())
	{
		GoHomeGameMode->ServerTravelViaLoadingScreen(TravelPath);
	}
}
