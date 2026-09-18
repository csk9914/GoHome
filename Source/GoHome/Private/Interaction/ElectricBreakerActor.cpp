


#include "Interaction/ElectricBreakerActor.h"
#include "Interaction/ElectricSwitchboardActor.h"
#include "Components/StaticMeshComponent.h"

AElectricBreakerActor::AElectricBreakerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BreakerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BreakerMesh"));
	SetRootComponent(BreakerMesh);
}


bool AElectricBreakerActor::CanInteract(APawn* InstigatorPawn) const
{
	return LinkedSwitchboard != nullptr;
}

void AElectricBreakerActor::OnInteract(APawn* InstigatorPawn)
{
	UE_LOG(LogTemp, Warning, TEXT("[Breaker] OnInteract 호출됨. LinkedSwitchboard=%s"), LinkedSwitchboard ? TEXT("있음") : TEXT("없음"));

	if (LinkedSwitchboard)
	{
		const bool bResolved = LinkedSwitchboard->TryResolveViaBreaker();
		UE_LOG(LogTemp, Warning, TEXT("[Breaker] TryResolveViaBreaker 결과: %s"), bResolved ? TEXT("성공") : TEXT("실패(퍼즐 미해결 상태)"));
	}
}

FText AElectricBreakerActor::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("차단기 내리기"));
}
