// 


#include "Core/LobbyGameMode.h"
#include "Core/LobbyGameState.h"


ALobbyGameMode::ALobbyGameMode()
{
	GameStateClass = ALobbyGameState::StaticClass();
}
