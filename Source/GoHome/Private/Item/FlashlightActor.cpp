
#include "Item/FlashlightActor.h"
#include "Item/FlashlightDataAsset.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Interaction/InventoryComponent.h"
#include "Player/GoHomeCharacter.h"

AFlashlightActor::AFlashlightActor()
{
	SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLight"));
	SpotLight->SetupAttachment(MeshComponent);
	SpotLight->SetVisibility(false);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(MeshComponent);
	PointLight->SetVisibility(false);
}

void AFlashlightActor::BeginPlay()
{
	Super::BeginPlay();
	SyncLightFromData();
}

void AFlashlightActor::SyncLightFromData()
{
	const UFlashlightDataAsset* FlashlightData = Cast<UFlashlightDataAsset>(ItemData);
	if (!FlashlightData) return;

	SpotLight->Intensity = FlashlightData->Intensity;
	SpotLight->SetLightColor(FlashlightData->LightColor);
	SpotLight->AttenuationRadius = FlashlightData->AttenuationRadius;
	SpotLight->SetOuterConeAngle(FlashlightData->OuterConeAngle);

	PointLight->Intensity = FlashlightData->Intensity;
	PointLight->SetLightColor(FlashlightData->LightColor);
	PointLight->AttenuationRadius = FlashlightData->AttenuationRadius;

	const bool bUseSpot = FlashlightData->FixtureType == ELightFixtureType::Spot;
	ActiveLight = bUseSpot ? Cast<USceneComponent>(SpotLight) : Cast<USceneComponent>(PointLight);

	UpdateLightVisual();
}

void AFlashlightActor::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !InstigatorPawn) return;

	UInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UInventoryComponent>();

	if (!Inventory) return;

	if (!Inventory->TryAddItem(this)) return;

	bIsBeingClaimed = true;
	HoldingPawn = InstigatorPawn;

	// 일반 아이템(ItemActorBase::OnInteract)과 달리 SetActiveSlot을 거치지 않는다.
	// 손전등은 슬롯은 차지하되, 활성 슬롯 여부와 무관하게 항상 왼손에 부착.
	// 오른손 아이템과 동시 사용 가능함(02문서 4절).
	UpdateAttachment();
}

void AFlashlightActor::UpdateAttachment(APawn* OldHoldinPawn)
{
	if (HoldingPawn)
	{
		MeshComponent->SetVisibility(true, false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (HasAuthority())
		{
			MeshComponent->SetSimulatePhysics(false);
		}

		if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn))
		{
			Character->AttachFlashlightToLeftHand(MeshComponent);
		}
	}

	else
	{
		MeshComponent->SetVisibility(true, false);
		MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		if (HasAuthority())
		{
			MeshComponent->SetSimulatePhysics(true);
			bIsOn = false; // 내려놓으면 자동 꺼짐.
			UpdateLightVisual();
		}

		if (AGoHomeCharacter* PrevCharacter = Cast<AGoHomeCharacter>(OldHoldinPawn))
		{
			PrevCharacter->DetachFlashlightFromLeftHand();
		}
	}
}

void AFlashlightActor::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn) return;

	bIsOn = !bIsOn;
	UpdateLightVisual();
}

void AFlashlightActor::OnRep_IsOn()
{
	UpdateLightVisual();
}

void AFlashlightActor::UpdateLightVisual()
{
	if (ActiveLight)
	{
		ActiveLight->SetVisibility(bIsOn);
	}
}

void AFlashlightActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlashlightActor, bIsOn);
}

FText AFlashlightActor::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("줍기"));
}

void AFlashlightActor::OnRep_ItemData()
{
	Super::OnRep_ItemData();
	SyncLightFromData();
}