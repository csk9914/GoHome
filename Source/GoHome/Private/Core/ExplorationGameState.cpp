// 


#include "Core/ExplorationGameState.h"
#include "Core/DockingDoorComponent.h"

AExplorationGameState::AExplorationGameState()
{
	// 생성자에서 대입하는 건 아직 리플리케이션이 시작되기 전이라 직접 대입
	// 어차피 이 시점엔 델리게이트 구독자(위젯 등)가 아직 없어서 브로드캐스트할 대상도 없음
	CurrentState = EExpeditionState::Exploration;
}

void AExplorationGameState::BeginPlay()
{
	Super::BeginPlay();
	
	DockingDoorComponent->SetOpen(true);
}
