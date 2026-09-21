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
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BlockerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockerMesh"));
	SetRootComponent(BlockerMesh);
	BlockerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockerMesh->SetCollisionObjectType(ECC_WorldStatic);

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

	// 블로커 콜리전이 꺼진 뒤에 스폰해야 아이템의 SnapToGround 트레이스가 문에 안 걸린다.
	SpawnRewards(Grade == ERewardGrade::High ? HighReward : RiskyReward);
}

void ARewardEntranceActor::OnRep_EntranceState()
{
	// 이미 begun play면 런타임 개방 -> 연출 재생. 아니면 늦게 접속한 클라의 상태 동기화 -> 반영만.
	ApplyEntranceState(HasActorBegunPlay());
}

void ARewardEntranceActor::ApplyEntranceState(bool bPlayEffects)
{
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

	// 열림: 문 숨김 + 통행 허용.
	BlockerMesh->SetVisibility(false);
	BlockerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BlockerMesh->SetCanEverAffectNavigation(false);

	if (!bPlayEffects) return;

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