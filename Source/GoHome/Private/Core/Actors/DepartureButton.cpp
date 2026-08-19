// 


#include "Core/Actors/DepartureButton.h"

#include "Core/ExpeditionZoneDataAsset.h"
#include "Core/GoHomeGameMode.h"
#include "Core/LobbyGameState.h"
#include "Core/DockingDoorComponent.h"


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

	FString TravelPath = "";

	//  현재 로비 맵일 때
	if (ALobbyGameState* LobbyGameState = GetWorld()->GetGameState<ALobbyGameState>())
	{
		// 탐사 맵으롷 이동
		const UExpeditionZoneDataAsset* SelectedZone = LobbyGameState->GetSelectedZone();
		if (!SelectedZone) return;

		TravelPath = SelectedZone->MapPath.ToSoftObjectPath().GetLongPackageName();
	}
	else if (AGoHomeGameState* GoHomeGameState = GetWorld()->GetGameState<AGoHomeGameState>())
	{
		if (UDockingDoorComponent* DoorComponent = GoHomeGameState->GetDockingDoorComponent())
		{
			DoorComponent->SetOpen(false);
		}
		TravelPath = GoHomeGameMode->GetLobbyMapPath();
	}

	GoHomeGameMode->ServerTravelViaLoadingScreen(TravelPath);
}
