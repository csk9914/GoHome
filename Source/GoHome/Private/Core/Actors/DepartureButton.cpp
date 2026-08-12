// 


#include "Core/Actors/DepartureButton.h"

#include "Core/ExpeditionZoneDataAsset.h"
#include "Core/GoHomeGameMode.h"
#include "Core/LobbyGameState.h"


ADepartureButton::ADepartureButton()
{
}

bool ADepartureButton::CanInteract(APawn* InstigatorPawn) const
{
	return true;
}

void ADepartureButton::OnInteract(APawn* InstigatorPawn)
{
	ALobbyGameState* LobbyGameState = GetWorld()->GetGameState<ALobbyGameState>();
	if (!LobbyGameState) return;
	
	const UExpeditionZoneDataAsset* SelectedZone = LobbyGameState->GetSelectedZone();
	if (!SelectedZone) return;
	
	if (AGoHomeGameMode* GoHomeGameMode = GetWorld()->GetAuthGameMode<AGoHomeGameMode>())
	{
		GoHomeGameMode->ServerTravelToMap(SelectedZone->MapPath.ToSoftObjectPath().ToString());
	}
	
	
}

