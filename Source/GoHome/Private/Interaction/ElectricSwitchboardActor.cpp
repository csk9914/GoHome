

#include "Interaction/ElectricSwitchboardActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Player/Stunnable.h"
#include "Player/GoHomeCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Interaction/SwitchboardPasswordWidget.h"
#include "Interaction/SwitchboardScreenWidget.h"
#include "Player/Damageable.h"
#include "Interaction/RewardEntranceActor.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "AI/NoiseType.h"

AElectricSwitchboardActor::AElectricSwitchboardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SwitchboardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwitchboardMesh"));
	SetRootComponent(SwitchboardMesh);
	SwitchboardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	EffectArea = CreateDefaultSubobject<UBoxComponent>(TEXT("EffectArea"));
	EffectArea->SetupAttachment(SwitchboardMesh);
	EffectArea->SetBoxExtent(FVector(300.f, 300.f, 300.f));
	EffectArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	EffectArea->SetGenerateOverlapEvents(true);

	FocusCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FocusCamera"));
	FocusCamera->SetupAttachment(SwitchboardMesh);

	MainScreenWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("MainScreenWidget"));
	MainScreenWidget->SetupAttachment(SwitchboardMesh);
	MainScreenWidget->SetWidgetSpace(EWidgetSpace::World);

	PasswordDisplayWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PasswordDisplayWidget"));
	PasswordDisplayWidget->SetupAttachment(SwitchboardMesh);
	PasswordDisplayWidget->SetWidgetSpace(EWidgetSpace::World);

	HumAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HumAudio"));
	HumAudio->SetupAttachment(SwitchboardMesh);
	HumAudio->bAutoActivate = false;
	HumAudio->SetIsReplicated(false); // 재생은 각 머신이 게이지 비율로 로컬 처리.

	WarningLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WarningLight"));
	WarningLight->SetupAttachment(SwitchboardMesh);
	WarningLight->SetMobility(EComponentMobility::Movable); // 런타임에 색/세기를 바꾸므로.


}


void AElectricSwitchboardActor::BeginPlay()
{
	Super::BeginPlay();

	// 조명은 에디터에서 위치/세기를 잡을 수 있게 켜둔 채로 두고, 시작 시 기준 세기를 캡처한 뒤 끈다.
	BaseLightIntensity = WarningLight->Intensity;
	WarningLight->SetVisibility(false);

	EffectArea->OnComponentBeginOverlap.AddDynamic(this, &AElectricSwitchboardActor::OnEffectAreaBeginOverlap);
	EffectArea->OnComponentEndOverlap.AddDynamic(this, &AElectricSwitchboardActor::OnEffectAreaEndOverlap);
	
	CollectKeypadComponents();

	if (USwitchboardScreenWidget* ScreenWidget = Cast<USwitchboardScreenWidget>(MainScreenWidget->GetUserWidgetObject()))
	{
		ScreenWidget->OwningSwitchboard = this;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switchboard] MainScreenWidget 참조 연결 실패 - GetUserWidgetObject: %s"),
			MainScreenWidget->GetUserWidgetObject() ? TEXT("존재하지만 Cast 실패") : TEXT("null"));
	}

	if (HasAuthority())
	{
		GenerateWirePuzzle();
	}
}

void AElectricSwitchboardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateDangerFeedback(DeltaTime); // 코스메틱: 서버/클라 모두 (전용 서버는 내부에서 제외).

	if (!HasAuthority() || SwitchboardState == ESwitchboardState::Resolved)
	{
		return;
	}

	const bool bShouldRiseGauge = (SwitchboardState == ESwitchboardState::Locked)
		                           || (OverlappingCharacters.Num() > 0)
	                               || (FocusingPawn != nullptr);

	if (!bShouldRiseGauge)
	{
		return;
	}

	DangerGauge = FMath::Clamp(DangerGauge + GaugeRisePerSecond * DeltaTime, 0.f, MaxGauge);

	TickAlarmNoise(DeltaTime);

	PenaltyTickAccumulator += DeltaTime;
	if (PenaltyTickAccumulator >= PenaltyTickInterval)
	{
		PenaltyTickAccumulator = 0.f;
		TickPenalizeOverlappingCharacters(DeltaTime);
	}
}

void AElectricSwitchboardActor::OnEffectAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
	                                                     AActor* OtherActor, 
	                                                     UPrimitiveComponent* OtherComp, 
	                                                     int32 OtherBodyIndex, 
	                                                     bool bFromSweep, 
	                                                     const FHitResult& SweepResult)
{
	if (ACharacter* OtherCharacter = Cast<ACharacter>(OtherActor))
	{
		OverlappingCharacters.AddUnique(OtherCharacter);
	}
}

void AElectricSwitchboardActor::OnEffectAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, 
	                                                   AActor* OtherActor, 
	                                                   UPrimitiveComponent* OtherComp, 
	                                                   int32 OtherBodyIndex)
{
	if (ACharacter* OtherCharacter = Cast<ACharacter>(OtherActor))
	{
		OverlappingCharacters.Remove(OtherCharacter);
		NextPenaltyEligibleTime.Remove(OtherCharacter);
	}
}



// TODO: DeathChance/StunChance 수치는 가안 - 다음 라운드에서 확정.
void AElectricSwitchboardActor::TickPenalizeOverlappingCharacters(float DeltaTime)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float GaugeRatio = DangerGauge / MaxGauge;
	const float DeathChance = GaugeRatio * 0.3f;
	const float StunChance = GaugeRatio * 0.5f;

	for (ACharacter* Character : OverlappingCharacters)
	{
		if (!IsValid(Character))
		{
			continue;
		}

		if (const float* EligibleTime = NextPenaltyEligibleTime.Find(Character))
		{
			if (Now < *EligibleTime)
			{
				continue; // 스턴 유예 중 -> 이번 틱은 건너뜀.
			}
		}

		if (!Character->Implements<UStunnable>())
		{
			continue;
		}

		const float Roll = FMath::FRand();
		const FVector KnockbackDir = (Character->GetActorLocation() - GetActorLocation()).GetSafeNormal();

		if (Roll < DeathChance)
		{
			if (UActorComponent* DamageableComponent = Character->FindComponentByInterface(UDamageable::StaticClass()))
			{
				IDamageable::Execute_ApplyDamage(DamageableComponent, 99999.f, this, TEXT("ElectricSwitchboard"));
			}
		}

		else if (Roll < DeathChance + StunChance)
		{
			IStunnable::Execute_ApplyStun(Character, StunDuration, KnockbackDir * KnockbackStrength, this);
			NextPenaltyEligibleTime.Add(Character, Now + StunDuration + PostStunGracePeriod);
		}
	}
}


void AElectricSwitchboardActor::GenerateWirePuzzle()
{
	PasswordDigits.Reset();

	const int32 DigitRange = WireTargets.Num(); // 10.

	for (int32 i = 0; i < PasswordLength; ++i)
	{
		PasswordDigits.Add(FMath::RandRange(0, DigitRange - 1));
	}
}

void AElectricSwitchboardActor::ServerSubmitPassword_Implementation(const TArray<int32>& EnterDigits, AActor* InInstigator)
{
	if (InInstigator != FocusingPawn) return;
	if (SwitchboardState != ESwitchboardState::PuzzleActive) return;

	if (EnterDigits == PasswordDigits)
	{
		HandlePuzzleSucceeded();
	}
	else
	{
		HandlePuzzleFailed();
	}
}


void AElectricSwitchboardActor::HandlePuzzleFailed()
{
	ReleaseFocus();
	SwitchboardState = ESwitchboardState::Locked;
	OnRep_SwitchboardState();

	// 자동 리셋 없음 - 실패하면 이 스테이지 안에서는 끝. 고위험 루트로만 보상 획득 가능.
	// (스테이지/라운드 재시작 시스템이 생기면 그때 ResetPuzzle()을 거기서 수동 호출)

	// 실패: 입구는 즉시 열리지만 게이지는 계속 오르고 구역 내 페널티도 계속됨 (Risky 등급 보상).
	for (ARewardEntranceActor* Entrance : LinkedEntrances)
	{
		if (Entrance)
		{
			Entrance->ServerOpen(ERewardGrade::Risky);
		}
	}
}

void AElectricSwitchboardActor::HandlePuzzleSucceeded()
{
	ReleaseFocus();
	SwitchboardState = ESwitchboardState::Resolved;
	OnRep_SwitchboardState();
}

bool AElectricSwitchboardActor::TryResolveViaBreaker()
{
	if (!CanFlipBreaker())
	{
		return false;
	}

	bBreakerFlipped = true; // 1회용 - 이후 차단기의 CanInteract가 false가 된다.
	DangerGauge = 0.f;

	// 성공: 차단기를 내리면 위험 해제 + 최고 등급 입구 오픈 (ServerOpen은 1회만 동작하므로 중복 조작에 안전).
	for (ARewardEntranceActor* Entrance : LinkedEntrances)
	{
		if (Entrance)
		{
			Entrance->ServerOpen(ERewardGrade::High);
		}
	}
	return true;
}

bool AElectricSwitchboardActor::CanInteract(APawn* InstigatorPawn) const
{
	if (SwitchboardState != ESwitchboardState::PuzzleActive) return false;
	return FocusingPawn == nullptr || FocusingPawn == InstigatorPawn;
}

void AElectricSwitchboardActor::OnInteract(APawn* InstigatorPawn)
{
	if (!FocusingPawn)
	{
		FocusingPawn = InstigatorPawn;
		SetOwner(InstigatorPawn); // ServerSubmitPassword RPC 라우팅에 필요.
	}

	if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(InstigatorPawn))
	{
		if (Character->IsLocallyControlled())
		{
			Character->EnterSwitchboardFocus(this);
		}
		else
		{
			Character->Client_EnterSwitchboardFocus(this);
		}
	}
}

FText AElectricSwitchboardActor::GetInteractionPromptText_Implementation() const
{
	if (FocusingPawn)
	{
		return FText::FromString(TEXT("사용 중"));
	}
	return FText::FromString(TEXT("배전반 조작"));
}

void AElectricSwitchboardActor::OnRep_DangerGauge() {}
void AElectricSwitchboardActor::OnRep_SwitchboardState() {}

void AElectricSwitchboardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AElectricSwitchboardActor, DangerGauge);
	DOREPLIFETIME(AElectricSwitchboardActor, SwitchboardState);
	DOREPLIFETIME(AElectricSwitchboardActor, PasswordDigits);
	DOREPLIFETIME(AElectricSwitchboardActor, FocusingPawn);
	DOREPLIFETIME(AElectricSwitchboardActor, bBreakerFlipped);
}

void AElectricSwitchboardActor::ReleaseFocus()
{
	if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(FocusingPawn.Get()))
	{
		if (Character->IsLocallyControlled())
		{
			Character->ExitSwitchboardFocus();
		}
		else
		{
			Character->Client_ExitSwitchboardFocus();
		}
	}

	FocusingPawn = nullptr;
	SetOwner(nullptr);
}

void AElectricSwitchboardActor::OnRep_FocusingPawn()
{
}

void AElectricSwitchboardActor::CollectKeypadComponents()
{
	WireTargets.Reset();

	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);

	TArray<TPair<int32, UStaticMeshComponent*>> Indexed;
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		const FString Name = Mesh->GetName();

		if (Name.StartsWith(TEXT("Wire")))
		{
			Indexed.Add({ FCString::Atoi(*Name.RightChop(4)), Mesh });
		}
		else if (Name == TEXT("KeypadBack"))
		{
			BackButtonMesh = Mesh;
		}
		else if (Name == TEXT("KeypadEnter"))
		{
			EnterButtonMesh = Mesh;
		}
	}

	Indexed.Sort([](const TPair<int32, UStaticMeshComponent*>& A, const TPair<int32, UStaticMeshComponent*>& B)
		{
			return A.Key < B.Key;
		});

	for (const TPair<int32, UStaticMeshComponent*>& Pair : Indexed)
	{
		WireTargets.Add(Pair.Value);
	}
}


void AElectricSwitchboardActor::ResetPuzzle()
{
	DangerGauge = 0.f;
	bBreakerFlipped = false;
	AlarmTimer = 0.f;
	AlarmSecondsUsed = 0.f;
	SwitchboardState = ESwitchboardState::PuzzleActive;
	OnRep_SwitchboardState();

	GenerateWirePuzzle();

}

UMeshComponent* AElectricSwitchboardActor::GetKeypadElementMesh(int32 Index) const
{
	if(WireTargets.IsValidIndex(Index)) return WireTargets[Index];
	if (Index == WireTargets.Num()) return BackButtonMesh;
	if (Index == WireTargets.Num() + 1) return EnterButtonMesh;
	return nullptr;
}

void AElectricSwitchboardActor::UpdatePasswordDisplay(const TArray<int32>& EnteredDigits)
{
	if (!PasswordDisplayWidget) return;

	if (USwitchboardPasswordWidget* Widget = Cast<USwitchboardPasswordWidget>(PasswordDisplayWidget->GetUserWidgetObject()))
	{
		Widget->SetEnteredDigits(EnteredDigits);
	}
}

void AElectricSwitchboardActor::UpdateDangerFeedback(float DeltaTime)
{
	if (IsNetMode(NM_DedicatedServer)) return; // 보고 들을 사람이 없음.

	// 효과 세기 = 실제 위험. Resolved는 게이지/페널티가 멈춘 상태라 0으로 간주.
	const float TargetRatio = (SwitchboardState == ESwitchboardState::Resolved || MaxGauge <= 0.f)
		? 0.f
		: FMath::Clamp(DangerGauge / MaxGauge, 0.f, 1.f);

	SmoothedFeedbackRatio = FMath::FInterpTo(SmoothedFeedbackRatio, TargetRatio, DeltaTime, FeedbackRatioInterpSpeed);
	if (TargetRatio <= 0.f && SmoothedFeedbackRatio < 0.01f)
	{
		SmoothedFeedbackRatio = 0.f; // 보간 꼬리를 잘라 완전히 꺼지게.
	}

	const float Ratio = SmoothedFeedbackRatio;
	const bool bActive = Ratio > 0.f;

	// 경고 조명
	if (WarningLight)
	{
		WarningLight->SetVisibility(bActive);

		if (bActive)
		{
			LightFlickerTimer -= DeltaTime;
			if (LightFlickerTimer <= 0.f)
			{
				LightFlickerTimer = FMath::FRandRange(0.02f, 0.12f);
				LightFlickerFactor = FMath::FRandRange(1.f - LightFlickerDepth * Ratio, 1.f);
			}

			WarningLight->SetLightColor((WarningLightColorLow * (1.f - Ratio)) + (WarningLightColorHigh * Ratio));
			WarningLight->SetIntensity(BaseLightIntensity * FMath::Lerp(WarningLightScaleMin, WarningLightScaleMax, Ratio) * LightFlickerFactor);
		}
	}

	// 험 루프
	if (HumAudio && HumAudio->Sound)
	{
		if (Ratio >= HumStartRatio)
		{
			const float HumAlpha = FMath::GetMappedRangeValueClamped(FVector2D(HumStartRatio, 1.f), FVector2D(0.f, 1.f), Ratio);
			HumAudio->SetVolumeMultiplier(FMath::Lerp(HumVolumeMin, HumVolumeMax, HumAlpha));
			HumAudio->SetPitchMultiplier(FMath::Lerp(HumPitchMin, HumPitchMax, HumAlpha));

			if (!HumAudio->IsPlaying())
			{
				HumAudio->Play();
			}
		}
		else if (HumAudio->IsPlaying())
		{
			HumAudio->Stop();
		}
	}

	// 스파크: 게이지가 높을수록 간격이 짧아짐.
	if (bActive)
	{
		SparkTimer -= DeltaTime;
		if (SparkTimer <= 0.f)
		{
			SparkTimer = FMath::Lerp(SparkIntervalMax, SparkIntervalMin, Ratio) * FMath::FRandRange(0.7f, 1.3f);
			SpawnSpark(Ratio);
		}
	}
}

void AElectricSwitchboardActor::SpawnSpark(float Ratio)
{
	const FVector Origin = SwitchboardMesh->Bounds.Origin;
	const FVector Extent = SwitchboardMesh->Bounds.BoxExtent * SparkAreaScale;
	const FVector Location = Origin + FVector(FMath::FRandRange(-Extent.X, Extent.X),
		FMath::FRandRange(-Extent.Y, Extent.Y),
		FMath::FRandRange(-Extent.Z, Extent.Z));

	if (SparkEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SparkEffect, Location, GetActorRotation());
	}

	if (SparkSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SparkSound, Location,
			FMath::Lerp(SparkVolumeMin, SparkVolumeMax, Ratio), FMath::FRandRange(0.9f, 1.1f), 0.f, SparkSoundAttenuation);
	}
}


void AElectricSwitchboardActor::TickAlarmNoise(float DeltaTime)
{
	if (MaxGauge <= 0.f) return;

	const float Ratio = FMath::Clamp(DangerGauge / MaxGauge, 0.f, 1.f);
	if (Ratio < AlarmStartRatio) return;

	// 누적 알람 상한: 소진되면 회로가 타서 이후 무음 (게이지/페널티는 그대로 유지).
	if (AlarmSecondsUsed >= MaxAlarmSeconds) return;
	AlarmSecondsUsed += DeltaTime;

	AlarmTimer -= DeltaTime;
	if (AlarmTimer > 0.f) return;

	AlarmTimer = FMath::Lerp(AlarmIntervalMax, AlarmIntervalMin, Ratio);

	ENoiseType Type = ENoiseType::Medium;
	float Radius = AlarmRadiusMedium;
	float Volume = AlarmVolumeMedium;
	float Pitch = AlarmPitchMedium;

	if (Ratio >= AlarmPeakRatio)
	{
		Type = ENoiseType::Alarm;
		Radius = AlarmRadiusPeak;
		Volume = AlarmVolumePeak;
		Pitch = AlarmPitchPeak;
	}
	else if (Ratio >= AlarmLargeRatio)
	{
		Type = ENoiseType::Large;
		Radius = AlarmRadiusLarge;
		Volume = AlarmVolumeLarge;
		Pitch = AlarmPitchLarge;
	}

	UGoHomeNoiseLibrary::GenerateNoise(this, GetActorLocation(), Radius, Type, this);

	// 소음이 발생하는 이 순간에 모든 머신이 같은 소리를 듣는다 (몬스터가 듣는 시점과 동기화).
	Multicast_PlayAlarmSound(Volume, Pitch);
}

void AElectricSwitchboardActor::Multicast_PlayAlarmSound_Implementation(float VolumeMultiplier, float PitchMultiplier)
{
	if (!AlarmSound || IsNetMode(NM_DedicatedServer)) return;

	UGameplayStatics::PlaySoundAtLocation(this, AlarmSound, GetActorLocation(),
		VolumeMultiplier, PitchMultiplier, 0.f, AlarmSoundAttenuation);
}