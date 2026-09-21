


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
	return LinkedSwitchboard != nullptr && LinkedSwitchboard->CanFlipBreaker();
}

void AElectricBreakerActor::OnInteract(APawn* InstigatorPawn)
{
	UE_LOG(LogTemp, Warning, TEXT("[Breaker] OnInteract 호출됨. LinkedSwitchboard=%s"), LinkedSwitchboard ? TEXT("있음") : TEXT("없음"));

	if (LinkedSwitchboard)
	{
		const bool bResolved = LinkedSwitchboard->TryResolveViaBreaker();
		UE_LOG(LogTemp, Warning, TEXT("[Breaker] TryResolveViaBreaker 결과: %s"), bResolved ? TEXT("성공") : TEXT("실패(퍼즐 미해결 또는 이미 조작됨)"));
	}
}

FText AElectricBreakerActor::GetInteractionPromptText_Implementation() const
{
	if (LinkedSwitchboard && LinkedSwitchboard->CanFlipBreaker())
	{
		return FText::FromString(TEXT("차단기 내리기"));
	}

	if (LinkedSwitchboard && LinkedSwitchboard->IsBreakerFlipped())
	{
		return FText::FromString(TEXT("차단기 (내려감)"));
	}

	return FText::FromString(TEXT("차단기 (전원 잠김)"));
}