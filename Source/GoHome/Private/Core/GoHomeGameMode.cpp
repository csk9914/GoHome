

#include "Core/GoHomeGameMode.h"
#include "Core/GoHomeGameState.h"

AGoHomeGameMode::AGoHomeGameMode()
{
	bUseSeamlessTravel = true;
}

void AGoHomeGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void AGoHomeGameMode::ServerTravelToMap(const FString& MapPath)
{
	GetWorld()->ServerTravel(MapPath + TEXT("?listen"), /*bAbsolute=*/true);
}
