//THE

#include "Interaction/InteractionComponent.h"
#include "Interaction/Interactable.h"
#include "Interaction/CoopCarryObjectBase.h"
#include "Player/GoHomeCharacter.h"
#include "Camera/CameraComponent.h"
#include "Interaction/DeliveryPoint.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Item/ItemActorBase.h"
#include "Kismet/GameplayStatics.h"

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
		PerformTrace(); // CurrenTarget 먼저 갱신함.
		UpdateNearbyItemHints(); // 갱신된 CurrentTarget을 제외하고 계산.
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
		SetOutlineEnabled(CurrentTarget, false, AimOutlineStencilValue);
		CurrentTarget = NewTarget;
		SetOutlineEnabled(CurrentTarget, true, AimOutlineStencilValue);

		OnInteractableTargetChanged.Broadcast(CurrentTarget);
	}
}

void UInteractionComponent::SetOutlineEnabled(AActor* Target, bool bEnabled, int32 StencilValue)
{
	if (!Target) return;

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Target->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		Component->SetRenderCustomDepth(bEnabled);
		if (bEnabled)
		{
			Component->SetCustomDepthStencilValue(StencilValue);
		}
	}
}

void UInteractionComponent::UpdateNearbyItemHints()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	TArray<AActor*> AllItems;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AItemActorBase::StaticClass(), AllItems);

	const FVector OwnerLocation = OwnerPawn->GetActorLocation();
	const float RadiusSquared = FMath::Square(NearbyHintRadius);

	TSet<TObjectPtr<AActor>> NewNearbyActors;

	for (AActor* Actor : AllItems)
	{
		if (!Actor || Actor == CurrentTarget) continue; // 조준 중인 대상은 제외(조준 강조 우선).

		AItemActorBase* Item = Cast<AItemActorBase>(Actor);
		if (!Item || !Item->CanInteract(OwnerPawn) || Item->HasBeenPickedUp()) continue; // 이미 누가 들고 있거나, 한 번 집혔던(버려진) 아이템은 제외.

		if (FVector::DistSquared(OwnerLocation, Actor->GetActorLocation()) <= RadiusSquared)
		{
			NewNearbyActors.Add(Actor);
		}
	}

	// 범위를 벗어났거나(또는 방금 조준 대상이 된) 아이템 -> 힌트 off.
	for (AActor* OldActor : NearbyHintedActors)
	{
		if (OldActor && !NewNearbyActors.Contains(OldActor) && OldActor != CurrentTarget)
		{
			SetOutlineEnabled(OldActor, false, NearbyHintStencilValue);
		}
	}

	// 새로 범위에 들어온 아이템 -> 힌트 on.
	for (AActor* NewActor : NewNearbyActors)
	{
		if (!NearbyHintedActors.Contains(NewActor))
		{
			SetOutlineEnabled(NewActor, true, NearbyHintStencilValue);
		}
	}
	NearbyHintedActors = NewNearbyActors;
}




void UInteractionComponent::TryInteract()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(OwnerPawn))
	{
		if (Character->IsCoopCarrying())
		{
			// 운반 중엔 납품 지점을 보고 있을 때만 반응(정산). 그 외엔 아무 것도 안 함(다른 상호작용 차단).
			if (Cast<ADeliveryPoint>(CurrentTarget))
			{
				Server_RequestDeliverCarry();
			}
			return;
		}
	}

	IInteractable* Interactable = Cast<IInteractable>(CurrentTarget);
	if (!Interactable) return;

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

void UInteractionComponent::Server_RequestDeliverCarry_Implementation()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(OwnerPawn))
	{
		if (ACoopCarryObjectBase* CarryObject = Character->GetCurrentCarryObject())
		{
			CarryObject->ServerDeliver();
		}
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





