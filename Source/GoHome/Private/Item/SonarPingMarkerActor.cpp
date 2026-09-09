


#include "Item/SonarPingMarkerActor.h"
#include "Components/StaticMeshComponent.h"

ASonarPingMarkerActor::ASonarPingMarkerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 코스메틱 전용 - 서버가 스폰해 내려보내는 게 아니라 각 클라가 직접 만듦
	bReplicates = false;

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	RootComponent = MarkerMesh;

	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetGenerateOverlapEvents(false);
}

void ASonarPingMarkerActor::InitMarker(float Lifetime)
{
	if (Lifetime > 0.f)
	{
		SetLifeSpan(Lifetime);
	}
}

