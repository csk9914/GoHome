#include "Item/RecoveryItemSpawnPoint.h"

#include "Components/SceneComponent.h"

ARecoveryItemSpawnPoint::ARecoveryItemSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}
