


#include "Item/SonarPingMarkerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"

ASonarPingMarkerActor::ASonarPingMarkerActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 코스메틱 전용 - 서버가 스폰해 내려보내는 게 아니라 각 클라가 직접 만듦
	bReplicates = false;

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	RootComponent = MarkerMesh;

	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetGenerateOverlapEvents(false);

	PingAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("PingAudio"));
	PingAudio->SetupAttachment(MarkerMesh);

	// 타이머로 직접 재생하므로 자동 시작은 끔
	PingAudio->bAutoActivate = false;
}

void ASonarPingMarkerActor::InitMarker(float Lifetime)
{
	if (Lifetime > 0.f)
	{
		SetLifeSpan(Lifetime);
	}
}

void ASonarPingMarkerActor::BeginPlay()
{
	Super::BeginPlay();

	if (PingInterval <= 0.f)
	{
		PingInterval = 1.f;
	}

	// 머티리얼 파동 주기를 소리 주기에 맞추고, 사이클 시작점을 스폰 시각에 고정
	// 둘 다 여기서 세팅해야 소리와 파동의 위상이 어긋나지 않음
	if (UMaterialInstanceDynamic* MID = MarkerMesh->CreateDynamicMaterialInstance(0))
	{
		MID->SetScalarParameterValue(TEXT("PulseSpeed"), 1.f / PingInterval);
		MID->SetScalarParameterValue(TEXT("SpawnTime"), GetWorld()->GetTimeSeconds());
	}

	PlayPingSound();

	GetWorldTimerManager().SetTimer(
		PingAudioTimerHandle,
		this,
		&ASonarPingMarkerActor::PlayPingSound,
		PingInterval,
		true);
}

void ASonarPingMarkerActor::PlayPingSound()
{
	if (PingAudio && PingAudio->Sound)
	{
		PingAudio->Play();
	}
}

