//

#include "Core/Actors/Submarine.h"

#include "Components/BoxComponent.h"

ASubmarine::ASubmarine()
{
	PrimaryActorTick.bCanEverTick = false;

	InteriorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteriorVolume"));
	SetRootComponent(InteriorVolume);

	// 플레이어 폰 존재 여부만 판정하면 되므로 쿼리 전용 + Pawn 채널만 오버랩 반응.
	InteriorVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteriorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteriorVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 기본 크기는 자리표시자. 에디터에서 잠수정 내부에 맞춰 조정한다.
	InteriorVolume->SetBoxExtent(FVector(200.f, 150.f, 100.f));
}
