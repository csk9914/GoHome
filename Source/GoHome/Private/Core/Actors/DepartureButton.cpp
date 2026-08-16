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
		// GoHomeGameMode->ServerTravelToMap(SelectedZone->MapPath.ToSoftObjectPath().ToString());
		
		// GetLongPackageName()은 오브젝트 이름 부분을 떼고 패키지 경로만 반환
		//GoHomeGameMode->ServerTravelToMap(SelectedZone->MapPath.ToSoftObjectPath().GetLongPackageName());
		GoHomeGameMode->ServerTravelViaLoadingScreen(SelectedZone->MapPath.ToSoftObjectPath().GetLongPackageName());
	}
	
	
}

