
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


void AFlashlightActor::Tick(float DeltaTime)
{
	if (HoldingPawn)
	{
		// 손에 들린 동안은 부력 로직(Super) 대신, 빛 회전을 풀레이어 시야 방향으로 계속 맟춰줌.
		// Camera 컴포넌트를 직접 읽으면 본인 화면에서만 정확하고 다른 클라이언트에겐 실시간으로 복제 되지 않음.
		// 리플리케이트 되는 ActorRotation(Yaw) + CurrentPitch 조합 사용.
		// 모든 클라이언트에서 각자 로컬로 계산 -> 서버 권위 체크 없음.
		if (bIsOn)
		{
			if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn))
			{
				const FRotator ViewRotation(Character->CurrentPitch, HoldingPawn->GetActorRotation().Yaw, 0.f);
				SpotLight->SetWorldRotation(ViewRotation);
			}
		}
		return;
	}

	Super::Tick(DeltaTime); //  드롭된 상태(부유/가라앉기)는 기존 부력 로직 그대로 수행.

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
		MeshComponent->SetCastShadow(false); // 손전등 그림자가 생기는 것 방지.

		if (HasAuthority())
		{
			MeshComponent->SetSimulatePhysics(false);
			CancelFloatCycle(); // 부력용 Tick은 끔.
		}

		// 빛 회전 동기화용으로 Tick은 계속 켜둬야함(서버+클라 전부 로컬로 계산 해야함 -> HasAuthority 밖에서 호출).
		SetActorTickEnabled(true);

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
		MeshComponent->SetCastShadow(true); // 월드에 놓인 손전등은 다시 정상적으로 그림자를 짐.

		if (HasAuthority())
		{
			MeshComponent->SetSimulatePhysics(true);
			bIsOn = false; // 내려놓으면 자동 꺼짐.
			UpdateLightVisual();
			BeginFloatCycle();
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