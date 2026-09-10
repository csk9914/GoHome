
#include "Interaction/CoopCarryObjectBase.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/GoHomeCharacter.h"
#include "Interaction/CoopCarryDataAsset.h"
#include "Core/GoHomeGameState.h"


ACoopCarryObjectBase::ACoopCarryObjectBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	NetDormancy = DORM_Awake;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	HandleA = CreateDefaultSubobject<USceneComponent>(TEXT("HandleA"));
	HandleA->SetupAttachment(MeshComponent);

	HandleB = CreateDefaultSubobject<USceneComponent>(TEXT("HandleB"));
	HandleB->SetupAttachment(MeshComponent);
}

void ACoopCarryObjectBase::BeginPlay()
{
	Super::BeginPlay();
	SyncFromCarryData();
}

void ACoopCarryObjectBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || !IsFullyCarried()) return;

	AGoHomeCharacter* CharacterA = Cast<AGoHomeCharacter>(CarrierA);
	AGoHomeCharacter* CharacterB = Cast<AGoHomeCharacter>(CarrierB);
	if (!CharacterA || !CharacterB) return;

	// 둘 사이 거리가 너무 벌어지면(한쪽이 억지로 멀어지려 하면) 강제로 놓침.
	if (FVector::Dist(CharacterA->GetActorLocation(), CharacterB->GetActorLocation()) > MaxCarryDistance ||
		FVector::Dist(GetActorLocation(), CharacterA->GetActorLocation()) > MaxCarryDistance ||
		FVector::Dist(GetActorLocation(), CharacterB->GetActorLocation()) > MaxCarryDistance)
	{
		ReleaseCarriers();
		return;
	}

	const FVector InputA = CharacterA->GetLastCarryInputWorld();
	const FVector InputB = CharacterB->GetLastCarryInputWorld();
	const FVector CombinedInput = (InputA + InputB) * 0.5f;
	
	// 두 캐리어 각자의 로컬 폰에서 이 값을 스스로 AddMovementInput 하도록 세팅만 함(직접 이동시키지 않음).
	CharacterA->SetCombinedCarryInput(CombinedInput * CarrySpeedScale);
	CharacterB->SetCombinedCarryInput(CombinedInput * CarrySpeedScale);

	// 오브젝트 자신은 두 캐리어의 중간점을 그대로 따라감(회전은 v1에서 고정, 추후 필요시 추가).
	const FVector Midpoint = (CharacterA->GetActorLocation() + CharacterB->GetActorLocation()) * 0.5f;
	SetActorLocation(Midpoint, true); // sweep = true -> 벽, 물건 등에 막히면 자연스럽게 안 뚫고 멈춤.
	ForceNetUpdate(); // 물리 없이 코드로 직접 옮기는 액터라, 다음 정기 갱신 주기를 안 기다리고 바로 리플리케이트 요청.

	// 두 캐리어를 잇는 축을 오브젝트 로컬 X축(HandleA -> HandleB)에 맞춰 회전
	// -> 누군가 앞서거나 옆으로 빠지면 자연스럽게 따라감.
	// HandleA : -X, HandleB : +X 배치.
	const FVector CarrierAxis = CharacterB->GetActorLocation() - CharacterA->GetActorLocation();
	if (!CarrierAxis.IsNearlyZero())
	{
		const FRotator TargetRotation = CarrierAxis.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, RotationInterpSpeed));
	}

	// 정체(손발 안 맞음) 판정 : 둘 다 뭔가 누르고 있는데 합산 결과는 작은 상태가 일정 시간 지속되면 강제로 놓침.
	const bool bBothPushing = InputA.SizeSquared() > FMath::Square(StuckInputThreshold) 
		&& InputB.SizeSquared() > FMath::Square(StuckInputThreshold);
	
	const bool bBarelyMoving = CombinedInput.SizeSquared() < FMath::Square(StuckInputThreshold);

	if (bBothPushing && bBarelyMoving)
	{
		TimeStuck += DeltaTime;
		if (TimeStuck >= StuckDropDuration)
		{
			ReleaseCarriers();
			TimeStuck = 0.f;
		}
	}

	else
	{
		TimeStuck = 0.f;
	}
}

void ACoopCarryObjectBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ServerDeliver()처럼 이미 정상적으로 놓아준 경우(CarrierA/B가 이미 nullptr)엔 중복 호출 안 됨.
	// 레벨 스트리밍 언로드/추락 등 예외적인 파괴 경로로도 캐리어 상태가 영구히 안 풀리는 것 방지.
	if (HasAuthority() && (CarrierA || CarrierB))
	{
		ReleaseCarriers();
	}

	Super::EndPlay(EndPlayReason);
}

bool ACoopCarryObjectBase::CanInteract(APawn* InstigatorPawn) const
{
	if (!InstigatorPawn) return false;

	// 빈 핸들이 하나도 없으면 못 잡음.
	if (CarrierA && CarrierB) return false;

	// 이미 다른 걸 운반 중인 폰은 못 잡음.
	if (const AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(InstigatorPawn))
	{
		if (Character->IsCoopCarrying()) return false;
	}

	return true;
}

void ACoopCarryObjectBase::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !InstigatorPawn) return;
	
	AssignCarrier(InstigatorPawn);
}

bool ACoopCarryObjectBase::AssignCarrier(APawn* Pawn)
{
	if (!CarrierA)
	{
		CarrierA = Pawn;
	}

	else if(!CarrierB)
	{
		CarrierB = Pawn;
	}

	else
	{
		return false; // 이미 둘 다 차있는 상태.
	}

	MeshComponent->SetSimulatePhysics(false); // 상호작용 동안은 물리 끄고 SetActorLocation으로만 이동.
	MeshComponent->IgnoreActorWhenMoving(Pawn, true); // 이동 스윕이 캐리어 본인 캡슐에 걸리는 것 방지.

	// 오브젝트가 캐릭터 이동보다 한 틱 늦게 계산되는 것 방지 -> 캐릭터 틱 이후에 실행되도록 순서 강제.
	AddTickPrerequisiteActor(Pawn);

	if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(Pawn))
	{
		Character->SetCoopCarryObject(this);
	}

	OnRep_Carriers(); // 서버 자신에게는 RepNotify가 안 뜨므로 직접 호출.
	return true;
}

void ACoopCarryObjectBase::ReleaseCarriers()
{
	if (!HasAuthority()) return;

	if (CarrierA)
	{ 
		RemoveTickPrerequisiteActor(CarrierA);
		MeshComponent->IgnoreActorWhenMoving(CarrierA, false); // 무시 해제.
	}

	if (CarrierB) 
	{ 
		RemoveTickPrerequisiteActor(CarrierB);
		MeshComponent->IgnoreActorWhenMoving(CarrierB, false);
	}

	if (AGoHomeCharacter* CharacterA = Cast<AGoHomeCharacter>(CarrierA))
	{
		CharacterA->SetCoopCarryObject(nullptr);
	}

	if (AGoHomeCharacter* CharacterB = Cast<AGoHomeCharacter>(CarrierB))
	{
		CharacterB->SetCoopCarryObject(nullptr);
	}

	CarrierA = nullptr;
	CarrierB = nullptr;
	TimeStuck = 0.f; // 다음 세션에 잔여 정체시간이 안 넘어가게.

	MeshComponent->SetSimulatePhysics(true); // 아무도 안잡고 있으면 물리 켜서 가라앉음.

	OnRep_Carriers();
}

void ACoopCarryObjectBase::OnRep_Carriers()
{
	// 캐릭터 운반 상태(bIsCoopCarrying) 갱신.
}

FText ACoopCarryObjectBase::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("함께 옮기기"));
}

void ACoopCarryObjectBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoopCarryObjectBase, CarrierA);
	DOREPLIFETIME(ACoopCarryObjectBase, CarrierB);
	DOREPLIFETIME(ACoopCarryObjectBase, CarryData);
	DOREPLIFETIME(ACoopCarryObjectBase, bIsBeingDelivered);
}

void ACoopCarryObjectBase::SyncFromCarryData()
{
	if (!CarryData) return;

	if (CarryData->Mesh)
	{
		MeshComponent->SetStaticMesh(CarryData->Mesh);
	}
	SetActorScale3D(CarryData->Scale);
}

void ACoopCarryObjectBase::OnRep_CarryData()
{
	SyncFromCarryData();
}

void ACoopCarryObjectBase::ServerDeliver()
{
	if (!HasAuthority() || bIsBeingDelivered || !CarryData || !IsFullyCarried()) return;
	bIsBeingDelivered = true;

	if (AGoHomeGameState* GameState = GetWorld()->GetGameState<AGoHomeGameState>())
	{
		GameState->AddDeliveredValue(FMath::RoundToInt(CarryData->Value));
	}

	ReleaseCarriers();
	Destroy();
}