
#include "Item/GlowItemActor.h"
#include "Item/GlowItemDataAsset.h"
#include "Components/PointLightComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/GoHomeCharacter.h"

AGlowItemActor::AGlowItemActor()
{
	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(MeshComponent);
	PointLight->SetVisibility(false);
}

void AGlowItemActor::BeginPlay()
{
	Super::BeginPlay();
	SyncLightFromData();
}


void AGlowItemActor::SyncLightFromData()
{
	const UGlowItemDataAsset* GlowData = Cast<UGlowItemDataAsset>(ItemData);
	if (!GlowData) return;

	PointLight->Intensity = GlowData->Intensity;
	PointLight->SetLightColor(GlowData->LightColor);
	PointLight->AttenuationRadius = GlowData->AttenuationRadius;
	PointLight->SetVolumetricScatteringIntensity(GlowData->VolumetricScatteringIntensity);

	if (RemainingGlowCharge < 0.f) // 최초 1회만 초기화 - 이미 소모중인 충전량을 덮어쓰지 않기 위함.
	{
		RemainingGlowCharge = GlowData->GlowChargeDuration;
	}

	UpdateLightVisual();
}

void AGlowItemActor::UpdateAttachment(APawn* OldHoldingPawn)
{
	if (HoldingPawn)
	{
		Super::UpdateAttachment(OldHoldingPawn); // 표준 오른손 활성/비활성 부착 로직 재활용.
		UpdateLightVisual(); // Super가 MeshComponent 자식(PointLight)까지 강제로 보이게 만들어서, bIsOn 기준으로 다시 맞춰줌.
		return;
	}

	// 드롭 처리 : 켜진 채로 드롭되면 충전량 소진될 때까지 계쏙 부유해야해서 Super를 안 쓰고 직접 구현함.
	MeshComponent->SetVisibility(true, true);
	MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCastShadow(true);

	if (HasAuthority())
	{
		MeshComponent->SetSimulatePhysics(true);

		if (!bIsOn)
		{
			BeginFloatCycle();
		}
		// 켜진 채 드롭이면 가라앉기 타이머를 안 걸고 계속 부유
		// -> TickGlowCharge()가 충전량 소진되는 순간 BeginFloatCycle()을 걸어줌.
	}

	UpdateLightVisual(); // 위 SetVisibility(true, true)로 강제 전파된 걸 bIsOn 기준으로 다시 맞춰줌.

	if (AGoHomeCharacter* PrevCharacter = Cast<AGoHomeCharacter>(OldHoldingPawn))
	{
		PrevCharacter->DetachItemFromRightHand();
	}
}

void AGlowItemActor::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn) return;
	if (!CanUse()) return;

	bIsOn = !bIsOn;
	UpdateLightVisual();
	UpdateGlowChargeTimer();
}

bool AGlowItemActor::CanUse() const
{
	// 끄는 건 항상 가능. 켜려는 시도인데 충전량이 없으면 불가(1회성 소모품).
	return bIsOn || RemainingGlowCharge > 0.f;
}

void AGlowItemActor::OnRep_IsOn()
{
	UpdateLightVisual();
}

void AGlowItemActor::UpdateLightVisual()
{
	if (PointLight)
	{
		PointLight->SetVisibility(bIsOn);
	}
}


void AGlowItemActor::UpdateGlowChargeTimer()
{
	if (bIsOn && RemainingGlowCharge > 0.f)
	{
		if (!GetWorldTimerManager().IsTimerActive(GlowChargeTimerHandle))
		{
			GetWorldTimerManager().SetTimer(GlowChargeTimerHandle, this, 
				&AGlowItemActor::TickGlowCharge, 0.25f, true);
		}

	}
	else
	{
		GetWorldTimerManager().ClearTimer(GlowChargeTimerHandle);
	}

}

void AGlowItemActor::TickGlowCharge()
{
	RemainingGlowCharge = FMath::Max(0.f, RemainingGlowCharge - 0.25f);

	const bool bShouldFlicker = RemainingGlowCharge > 0.f && RemainingGlowCharge <= FlickerThreshold;

	if (bShouldFlicker != bIsFlickering)
	{
		bIsFlickering = bShouldFlicker;
		OnRep_IsFlickering(); // 서버 자신에게는 RepNotify가 안 뜨므로 직접 호출함.
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


void AGlowItemActor::OnRep_IsFlickering()
{
	if (bIsFlickering)
	{
		ToggleFlickerVisual();
	}
	else
	{
		GetWorldTimerManager().ClearTimer(FlickerTimerHandle);
		UpdateLightVisual();
	}
}

void AGlowItemActor::ToggleFlickerVisual()
{
	if (!bIsFlickering || !PointLight) return;

	PointLight->SetVisibility(!PointLight->IsVisible());
	GetWorldTimerManager().SetTimer(FlickerTimerHandle, this, 
		&AGlowItemActor::ToggleFlickerVisual, FMath::FRandRange(0.05f, 0.15f), false);

}

void AGlowItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGlowItemActor, bIsOn);
	DOREPLIFETIME(AGlowItemActor, bIsFlickering);
}

void AGlowItemActor::OnRep_ItemData()
{
	Super::OnRep_ItemData();
	SyncLightFromData();
}

