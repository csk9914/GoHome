//THE

#include "Interaction/InteractionComponent.h"
#include "Interaction/Interactable.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedCamera = GetOwner()->FindComponentByClass<UCameraComponent>();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	TimeSinceLastTrace += DeltaTime;
	if (TimeSinceLastTrace >= TraceInterval)
	{
		TimeSinceLastTrace = 0.f;
		PerformTrace();
	}
}

void UInteractionComponent::PerformTrace()
{
	if (!CachedCamera) return;

	const FVector Start = CachedCamera->GetComponentLocation();
	const FVector End = Start + CachedCamera->GetForwardVector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner()); // 자기 자신은 무시.

	AActor* NewTarget = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (Hit.GetActor() && Hit.GetActor()->Implements<UInteractable>())
		{
			NewTarget = Hit.GetActor();
		}
	}

	if (NewTarget != CurrentTarget)
	{
		CurrentTarget = NewTarget;

		OnInteractableTargetChanged.Broadcast(CurrentTarget);
	}
}



void UInteractionComponent::TryInteract()
{
	IInteractable* Interactable = Cast<IInteractable>(CurrentTarget);
	if (!Interactable) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (Interactable->CanInteract(OwnerPawn))
	{
		// TODO(Phase 2): OnInteract가 아직 빈 스텁이라 지금은 로컬 직접 호출이 안전하지만,
		// 실제 픽업 로직이 들어가면 클라이언트가 직접 호출하면 안 됨
		// Server_RequestInteract(CurrentTarget) 서버 RPC로 교체 필요
		// (02문서 4절 "소유권 이전은 서버 RPC 요청 후 승인").

		Interactable->OnInteract(OwnerPawn);
	}

}









