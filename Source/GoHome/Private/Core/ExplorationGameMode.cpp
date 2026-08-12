// 


#include "Core/ExplorationGameMode.h"
#include "Core/ExplorationGameState.h"

AExplorationGameMode::AExplorationGameMode()
{
	GameStateClass = AExplorationGameState::StaticClass();
}
