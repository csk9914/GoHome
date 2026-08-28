//THE

#include "Interaction/InteractionComponent.h"
#include "Interaction/Interactable.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Server RPC를 쓰려면 컴포넌트 자체가 리플리케이트 되어야 한다.
	// 생성자(CDO 초기화) 시점에는 SetIsReplicated 대신 SetIsReplicatedByDefault를 써야 ensure가 안 뜬다.
	SetIsReplicatedByDefault(true);
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
		SetOutlineEnabled(CurrentTarget, false);
		CurrentTarget = NewTarget;
		SetOutlineEnabled(CurrentTarget, true);

		OnInteractableTargetChanged.Broadcast(CurrentTarget);
	}
}

void UInteractionComponent::SetOutlineEnabled(AActor* Target, bool bEnabled)
{
	if (!Target) return;

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Target->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		Component->SetRenderCustomDepth(bEnabled);
		if (bEnabled)
		{
			Component->SetCustomDepthStencilValue(OutlineStencilValue);
		}
	}
}





void UInteractionComponent::TryInteract()
{
	IInteractable* Interactable = Cast<IInteractable>(CurrentTarget);
	if (!Interactable) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (Interactable->CanInteract(OwnerPawn))
	{
		Server_RequestInteract(CurrentTarget);
	}
}

void UInteractionComponent::Server_RequestInteract_Implementation(AActor* Target)
{
	IInteractable* Interactable = Cast<IInteractable>(Target);

	if (!Interactable) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	// 서버에서 다시 검증 -> 클라이언트가 보낸 요청을 그대로 신뢰하지 않는다.
	if (Interactable->CanInteract(OwnerPawn))
	{
		Interactable->OnInteract(OwnerPawn);
	}

}


FText UInteractionComponent::GetInteractionPromptTextFor(AActor* Target)
{
	if (Target && Target->Implements<UInteractable>())
	{
		const FText PromptText = IInteractable::Execute_GetInteractionPromptText(Target);
		if (!PromptText.IsEmpty())
		{
			return PromptText;
		}

	}

	return FText::FromString(TEXT("상호작용"));
}





