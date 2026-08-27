
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
	SpotLight->SetInnerConeAngle(FlashlightData->InnerConeAngle);
	SpotLight->SetVolumetricScatteringIntensity(FlashlightData->VolumetricScatteringIntensity);

	PointLight->Intensity = FlashlightData->Intensity;
	PointLight->SetLightColor(FlashlightData->LightColor);
	PointLight->AttenuationRadius = FlashlightData->AttenuationRadius;
	PointLight->SetVolumetricScatteringIntensity(FlashlightData->VolumetricScatteringIntensity);

	const bool bUseSpot = FlashlightData->FixtureType == ELightFixtureType::Spot;
	ActiveLight = bUseSpot ? Cast<USceneComponent>(SpotLight) : Cast<USceneComponent>(PointLight);

	if (RemainingGlowCharge < 0.f) // 최초 1회만 초기화 - 이미 소모 중인 충전량을 덮어쓰지 않기 위함.
	{
		RemainingGlowCharge = FlashlightData->GlowChargeDuration;
	}

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

			const UFlashlightDataAsset* FlashlightData = Cast<UFlashlightDataAsset>(ItemData);
			const bool bIsOmni = FlashlightData && FlashlightData->FixtureType == ELightFixtureType::Omni;

			if (!bIsOmni)
			{
				bIsOn = false; // Spot은 내려놓으면 꺼짐.
				UpdateLightVisual();
			}

			// Omni는 켜진 상태를 그대로 유지함 -> 충전량이 소모될 때 까지 유지(bIsOn은 안 건드림).

			// Omni가 켜진 채로 드롭되면, 꺼질 때 까지는 가라앉기 타이머를 걸지 않고 계속 부유상태 지속.
			// 충전시간이 지난 후 꺼지는 순간(TickGlowCharge)에 대신 걸어둠.
			if(!(bIsOmni && bIsOn))
			{
				BeginFloatCycle();
			}
			
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

	const UFlashlightDataAsset* FlashlightData = Cast<UFlashlightDataAsset>(ItemData);
	const bool bIsOmni = FlashlightData && FlashlightData->FixtureType == ELightFixtureType::Omni;

	// Omni 는 활성 슬롯일 때만 F Key에 반응
	// 안그러면 Spot Light랑 같이 들고 있을 때 Spot을 킬 때 Omni까지 같이 켜짐(인벤토리 전체에 F가 브로드캐스트 되는 구조).
	if (bIsOmni && !bIsActiveHeld) return;

	// Omni는 충전량을 다 쓰면 다시 킬 수 없음.
	if (bIsOmni && !bIsOn && RemainingGlowCharge <= 0.f) return;

	bIsOn = !bIsOn;
	UpdateLightVisual();
	UpdateGlowChargeTimer();
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

void AFlashlightActor::UpdateGlowChargeTimer()
{
	const UFlashlightDataAsset* FlashlightData = Cast<UFlashlightDataAsset>(ItemData);
	const bool bIsOmni = FlashlightData && FlashlightData->FixtureType == ELightFixtureType::Omni;

	if (bIsOmni && bIsOn && RemainingGlowCharge > 0.f)
	{
		if (!GetWorldTimerManager().IsTimerActive(GlowChargeTimerHandle))
		{
			GetWorldTimerManager().SetTimer(GlowChargeTimerHandle, this, &AFlashlightActor::TickGlowCharge, 0.25f, true);

		}
	}
	else
	{
		GetWorldTimerManager().ClearTimer(GlowChargeTimerHandle);
	}

}

void AFlashlightActor::TickGlowCharge()
{
	RemainingGlowCharge = FMath::Max(0.f, RemainingGlowCharge - 0.25f);

	const bool bShouldFlicker = RemainingGlowCharge > 0.f && RemainingGlowCharge <= FlickerThreshold;

	if (bShouldFlicker != bIsFlickering)
	{
		bIsFlickering = bShouldFlicker;
		OnRep_IsFlickering(); // 서버 자신에게는 RepNotify가 안 뜨므로 직접 호출.
	}

	if (RemainingGlowCharge <= 0.f)
	{
		GetWorldTimerManager().ClearTimer(GlowChargeTimerHandle);
		bIsOn = false;
		bIsFlickering = false;
		OnRep_IsFlickering();
		UpdateLightVisual();
		BeginFloatCycle(); // 꺼지는 순간부터 가라앉기/떠오르기 사이클 시작.
	}
}

void AFlashlightActor::OnRep_IsFlickering()
{
	if (bIsFlickering)
	{
		ToggleFlickerVisual();
	}
	else
	{
		GetWorldTimerManager().ClearTimer(FlickerTimerHandle);
		UpdateLightVisual(); // 깜빡임 종료 -> bIsOn 기준 정상 상태로 복원.
	}
}

void AFlashlightActor::ToggleFlickerVisual()
{
	if (!bIsFlickering || !ActiveLight) return;

	ActiveLight->SetVisibility(!ActiveLight->IsVisible());
	GetWorldTimerManager().SetTimer(FlickerTimerHandle, this, 
		&AFlashlightActor::ToggleFlickerVisual, FMath::FRandRange(0.05f, 0.15f), false);
}


void AFlashlightActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlashlightActor, bIsOn);
	DOREPLIFETIME(AFlashlightActor, bIsFlickering);
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