

#include "AI/MonsterBase.h"

AMonsterBase::AMonsterBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMonsterBase::OnNoiseHeard_Implementation(FVector Location, float Radius, ENoiseType Type, AActor* Source)
{
}

void AMonsterBase::Attack_Implementation(AActor* Target)
{
}
