

#include "AI/NoiseType.h"
#include "AI/MonsterNoiseListener.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UGoHomeNoiseLibrary::GenerateNoise(
	const UObject* WorldContextObject, 
	FVector Location, 
	float Radius, 
	ENoiseType Type, 
	AActor* Source)
{
	if (!WorldContextObject)
	{
		return;
	}
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		return;
	}
	TArray<AActor*> Listeners;
	UGameplayStatics::GetAllActorsWithInterface(
		World,
		UMonsterNoiseListener::StaticClass(),
		Listeners
	);
	for (AActor* Listener : Listeners)
	{
		if (!Listener)
		{
			continue;
		}
		const float Distance = FVector::Dist(
			Listener->GetActorLocation(),
			Location
		);
		if (Distance <= Radius)
		{
			IMonsterNoiseListener::Execute_OnNoiseHeard(
				Listener,
				Location,
				Radius,
				Type,
				Source
			);
		}
	}
}
