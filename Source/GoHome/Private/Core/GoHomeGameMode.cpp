#include "Core/GoHomeGameMode.h"

#include "Core/ExpeditionTravelSubsystem.h"
#include "Core/GoHomeGameState.h"
#include "Core/GoHomePlayerController.h"

AGoHomeGameMode::AGoHomeGameMode()
{
	bUseSeamlessTravel = true;

	PlayerControllerClass = AGoHomePlayerController::StaticClass();
}

void AGoHomeGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void AGoHomeGameMode::ServerTravelToMap(const FString& MapPath)
{
	// UE_LOG(LogTemp, Warning, TEXT("[GoHome] ServerTravelToMap called: %s"), *MapPath);

	if (AGoHomeGameState* GoHomeGameState = GetGameState<AGoHomeGameState>())
	{
		GoHomeGameState->SetState(EExpeditionState::Departure);
	}

	GetWorld()->ServerTravel(MapPath + TEXT("?listen"), /*bAbsolute=*/true);
}

void AGoHomeGameMode::ServerTravelViaLoadingScreen(const FString& FinalMapPath)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UExpeditionTravelSubsystem* TravelSubsystem = GameInstance->GetSubsystem<UExpeditionTravelSubsystem>())
		{
			TravelSubsystem->PendingDestinationMap = FinalMapPath;
		}
	}

	ServerTravelToMap(LoadingMapPath);
}
