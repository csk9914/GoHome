#include "Core/GoHomeGameMode.h"

#include "Core/ExpeditionTravelSubsystem.h"
#include "Core/GoHomeGameState.h"
#include "Core/GoHomePlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

AGoHomeGameMode::AGoHomeGameMode()
{
	bUseSeamlessTravel = true;

	PlayerControllerClass = AGoHomePlayerController::StaticClass();
}

void AGoHomeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!NewPlayer)
	{
		return;
	}

	APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>();
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!PlayerState || !PlayerState->GetUniqueId().IsValid() || !OnlineSubsystem)
	{
		return;
	}

	const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		return;
	}

	FString Nickname = Identity->GetPlayerNickname(*PlayerState->GetUniqueId());
	Nickname.TrimStartAndEndInline();
	if (!Nickname.IsEmpty())
	{
		Nickname.ReplaceInline(TEXT("\r"), TEXT(""));
		Nickname.ReplaceInline(TEXT("\n"), TEXT(""));
		PlayerState->SetPlayerName(Nickname.Left(32));
	}
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
			TravelSubsystem->SetPendingDestinationMap(FinalMapPath);
		}
	}

	ServerTravelToMap(LoadingMapPath);
}
