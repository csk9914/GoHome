#include "Interaction/RewardEntranceActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"



ARewardEntranceActor::ARewardEntranceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 문 개방 연출 동안에만 켠다.
	bReplicates = true;

	BlockerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockerMesh"));
	SetRootComponent(BlockerMesh);
	BlockerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockerMesh->SetCollisionObjectType(ECC_WorldStatic);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(BlockerMesh);
	DoorMesh->SetMobility(EComponentMobility::Movable); // 런타임에 움직이므로.
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorMesh->SetCollisionObjectType(ECC_WorldStatic);

	GapGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("GapGlow"));
	GapGlow->SetupAttachment(BlockerMesh);
	GapGlow->SetMobility(EComponentMobility::Movable); // 런타임에 색/세기를 바꾸므로.
}

void ARewardEntranceActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyEntranceState(false); // 초기(봉인) 글로우 반영. 이미 열린 상태로 접속한 클라도 여기서 정리됨.
}

void ARewardEntranceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARewardEntranceActor, EntranceState);
}

void ARewardEntranceActor::ServerOpen(ERewardGrade Grade)
{
	if (!HasAuthority() || EntranceState != EEntranceState::Sealed) return;

	EntranceState = (Grade == ERewardGrade::High) ? EEntranceState::OpenedHigh : EEntranceState::OpenedRisky;
	UE_LOG(LogTemp, Warning, TEXT("[RewardEntrance] 열림 - 등급: %s"), Grade == ERewardGrade::High ? TEXT("High") : TEXT("Risky"));

	ApplyEntranceState(true); // 서버 자신은 OnRep이 안 뜨므로 수동 호출.

	// 문 콜리전은 열린 뒤에도 문과 함께 남는다 - 보상 위치(Reward*)는 닫힌 문 콜리전에서 충분히 떨어져 있어야 SnapToGround 트레이스가 문에 안 걸림(LD 규칙).
	SpawnRewards(Grade == ERewardGrade::High ? HighReward : RiskyReward);
}

void ARewardEntranceActor::OnRep_EntranceState()
{
	// 이미 begun play면 런타임 개방 -> 연출 재생. 아니면 늦게 접속한 클라의 상태 동기화 -> 반영만.
	ApplyEntranceState(HasActorBegunPlay());
}

void ARewardEntranceActor::ApplyEntranceState(bool bPlayEffects)
{
	CaptureDoorClosedPose(); // 문이 한 번도 안 움직인 첫 호출에서 닫힌 자세를 캡처.

	if (BaseGlowIntensity < 0.f)
	{
		BaseGlowIntensity = GapGlow->Intensity;
	}

	FLinearColor GlowColor = SealedGlowColor;
	float GlowScale = SealedGlowScale;
	UNiagaraSystem* Effect = nullptr;
	USoundBase* Sound = nullptr;

	switch (EntranceState)
	{
	case EEntranceState::OpenedHigh:
		GlowColor = HighGlowColor;
		GlowScale = HighGlowScale;
		Effect = HighOpenEffect;
		Sound = HighOpenSound;
		break;
	case EEntranceState::OpenedRisky:
		GlowColor = RiskyGlowColor;
		GlowScale = RiskyGlowScale;
		Effect = RiskyOpenEffect;
		Sound = RiskyOpenSound;
		break;
	default:
		break;
	}

	GapGlow->SetLightColor(GlowColor);
	GapGlow->SetIntensity(BaseGlowIntensity * GlowScale);

	if (EntranceState == EEntranceState::Sealed) return;

	if (!bPlayEffects)
	{
		// 늦게 접속한 클라의 동기화 - 연출 없이 열린 최종 자세로.
		SetDoorOpenAlpha(1.f);
		DoorMesh->SetVisibility(!bHideDoorAfterOpen);
		return;
	}

	// 문 개방 연출 시작 (Tick이 DoorOpenDuration 동안만 돈다).
	DoorOpenElapsed = 0.f;
	SetActorTickEnabled(true);

	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect, GetActorLocation(), GetActorRotation());
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}


void ARewardEntranceActor::SpawnRewards(const FRewardTierConfig& Config)
{
	TArray<UItemDataAsset*> ValidPool;
	for (UItemDataAsset* Data : Config.Pool)
	{
		if (Data)
		{
			ValidPool.Add(Data);
		}
	}

	if (ValidPool.Num() == 0 || Config.Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RewardEntrance] 보상 풀이 비었거나 Count가 0 - 스폰 없음"));
		return;
	}

	// "Reward0", "Reward1"... 이름의 씬 컴포넌트를 스폰 위치로 사용 (배전반 Wire와 같은 이름 규칙).
	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);

	TArray<FTransform> SpawnTransforms;
	for (USceneComponent* Component : Components)
	{
		if (Component->GetName().StartsWith(TEXT("Reward")))
		{
			SpawnTransforms.Add(Component->GetComponentTransform());
		}
	}

	if (SpawnTransforms.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RewardEntrance] Reward0..N 스폰 위치 컴포넌트가 없음 - 액터 위치에 스폰"));
		SpawnTransforms.Add(GetActorTransform());
	}

	if (Config.Count > SpawnTransforms.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RewardEntrance] Count(%d) > 스폰 위치(%d) - 위치 수만큼만 스폰"), Config.Count, SpawnTransforms.Num());
	}

	const int32 SpawnCount = FMath::Min(Config.Count, SpawnTransforms.Num());

	for (int32 i = 0; i < SpawnCount; ++i)
	{
		UItemDataAsset* Picked = ValidPool[FMath::RandRange(0, ValidPool.Num() - 1)];
		const FTransform& SpawnTransform = SpawnTransforms[i];

		AItemActorBase* SpawnedItem = GetWorld()->SpawnActorDeferred<AItemActorBase>(
			AItemActorBase::StaticClass(), SpawnTransform);

		if (SpawnedItem)
		{
			SpawnedItem->ItemData = Picked;
			SpawnedItem->FinishSpawning(SpawnTransform);
			SpawnedItem->ForceNetUpdate();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[RewardEntrance] 보상 %d개 스폰"), SpawnCount);
}

void ARewardEntranceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DoorOpenElapsed += DeltaTime;
	const float RawAlpha = (DoorOpenDuration > 0.f) ? FMath::Clamp(DoorOpenElapsed / DoorOpenDuration, 0.f, 1.f) : 1.f;
	SetDoorOpenAlpha(FMath::InterpEaseInOut(0.f, 1.f, RawAlpha, DoorEaseExponent));

	if (RawAlpha >= 1.f)
	{
		SetActorTickEnabled(false);
		DoorMesh->SetVisibility(!bHideDoorAfterOpen);
	}
}

void ARewardEntranceActor::CaptureDoorClosedPose()
{
	if (bDoorClosedPoseCaptured) return;

	bDoorClosedPoseCaptured = true;
	DoorClosedLocation = DoorMesh->GetRelativeLocation();
	DoorClosedRotation = DoorMesh->GetRelativeRotation().Quaternion();
}

void ARewardEntranceActor::SetDoorOpenAlpha(float Alpha)
{
	DoorMesh->SetRelativeLocation(DoorClosedLocation + DoorOpenLocationOffset * Alpha);
	DoorMesh->SetRelativeRotation(DoorClosedRotation * (DoorOpenRotationOffset * Alpha).Quaternion());
}