

#include "Interaction/ElectricSwitchboardActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Player/Stunnable.h"
#include "Player/GoHomeCharacter.h"
#include "Player/Damageable.h"

AElectricSwitchboardActor::AElectricSwitchboardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	EffectArea = CreateDefaultSubobject<UBoxComponent>(TEXT("EffectArea"));
	SetRootComponent(EffectArea);
	EffectArea->SetBoxExtent(FVector(300.f, 300.f, 300.f));
	EffectArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	EffectArea->SetGenerateOverlapEvents(true);

	SwitchboardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwitchboardMesh"));
	SwitchboardMesh->SetupAttachment(EffectArea);
	SwitchboardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}


void AElectricSwitchboardActor::BeginPlay()
{
	Super::BeginPlay();

	EffectArea->OnComponentBeginOverlap.AddDynamic(this, &AElectricSwitchboardActor::OnEffectAreaBeginOverlap);
	EffectArea->OnComponentEndOverlap.AddDynamic(this, &AElectricSwitchboardActor::OnEffectAreaEndOverlap);
	
	CollectWireTargets();

	if (HasAuthority())
	{
		GenerateWirePuzzle();
	}
}

void AElectricSwitchboardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || SwitchboardState == ESwitchboardState::Resolved)
	{
		return;
	}

	if (OverlappingCharacters.Num() == 0)
	{
		return; // 아무도 없으면 게이지도 안 오르고 페널티도 없음.
	}

	DangerGauge = FMath::Clamp(DangerGauge + GaugeRisePerSecond * DeltaTime, 0.f, MaxGauge);

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
		UE_LOG(LogTemp, Warning, TEXT("[Switchboard] 판정: Gauge=%.1f Roll=%.2f Death=%.2f Stun=%.2f"), DangerGauge, Roll, DeathChance, StunChance);
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
	WireRemovalOrder.Reset();
	for (int32 i = 0; i < WireTargets.Num(); ++i)
	{
		WireRemovalOrder.Add(i);
	}

	for (int32 i = WireRemovalOrder.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		WireRemovalOrder.Swap(i, j);
	}

	HintMode = FMath::RandBool() ? EWireHintMode::FlashSequence : EWireHintMode::ColorIndexClue;
	NextCorrectStep = 0;
}

void AElectricSwitchboardActor::ServerRemoveWire_Implementation(int32 WireIndex, AActor* InInstigator)
{
	if (InInstigator != FocusingPawn) return;

	if (SwitchboardState != ESwitchboardState::PuzzleActive || !WireRemovalOrder.IsValidIndex(NextCorrectStep))
	{
		return;
	}

	if (WireIndex == WireRemovalOrder[NextCorrectStep])
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switchboard] 정답! (%d/%d)"), NextCorrectStep + 1, WireRemovalOrder.Num());
		++NextCorrectStep;
		OnRep_NextCorrectStep();

		if (NextCorrectStep >= WireRemovalOrder.Num())
		{
			HandlePuzzleSucceeded();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Switchboard] 오답! 정답 %d / 선택 %d"), WireRemovalOrder[NextCorrectStep], WireIndex);
		HandlePuzzleFailed();
	}
}

void AElectricSwitchboardActor::HandlePuzzleFailed()
{
	UE_LOG(LogTemp, Warning, TEXT("[Switchboard] 퍼즐 실패"));
	ReleaseFocus();
	SwitchboardState = ESwitchboardState::Locked;
	OnRep_SwitchboardState();
	// TODO: 고위험 입구 오픈 로직 - 다음 라운드.
}

void AElectricSwitchboardActor::HandlePuzzleSucceeded()
{
	UE_LOG(LogTemp, Warning, TEXT("[Switchboard] 퍼즐 성공"));
	ReleaseFocus();
	SwitchboardState = ESwitchboardState::Resolved;
	OnRep_SwitchboardState();
}

bool AElectricSwitchboardActor::TryResolveViaBreaker()
{
	if (SwitchboardState != ESwitchboardState::Resolved)
	{
		return false;
	}

	DangerGauge = 0.f;
	// TODO: 최고 보상 입구 오픈 로직 - 다음 라운드.
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
		SetOwner(InstigatorPawn); // ServerRemoveWire RPC 라우팅에 필요.
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
void AElectricSwitchboardActor::OnRep_NextCorrectStep() {}

void AElectricSwitchboardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AElectricSwitchboardActor, DangerGauge);
	DOREPLIFETIME(AElectricSwitchboardActor, SwitchboardState);
	DOREPLIFETIME(AElectricSwitchboardActor, WireRemovalOrder);
	DOREPLIFETIME(AElectricSwitchboardActor, HintMode);
	DOREPLIFETIME(AElectricSwitchboardActor, NextCorrectStep);
	DOREPLIFETIME(AElectricSwitchboardActor, FocusingPawn);
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

void AElectricSwitchboardActor::CollectWireTargets()
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