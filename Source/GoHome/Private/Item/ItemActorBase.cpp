

#include "Item/ItemActorBase.h"
#include "Item/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "AI/NoiseType.h"
#include "TimerManager.h"
#include "Interaction/InventoryComponent.h"
#include "Player/GoHomeCharacter.h"

AItemActorBase::AItemActorBase()
{
	PrimaryActorTick.bCanEverTick = false; // 진단용 true였던 거 원복
	bReplicates = true;

	// AActor는 Pawn/Character와 달리 bReplicateMovement가 꺼져 있는 것으로 보임. 그래서 생성자에서 켜줘야함.
	SetReplicateMovement(true); 

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetIsReplicated(false);
	MeshComponent->SetNotifyRigidBodyCollision(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultMeshAsset.Object);
	}
}


void AItemActorBase::BeginPlay()
{
	Super::BeginPlay();

	MeshComponent->SetSimulatePhysics(HasAuthority());
}

bool AItemActorBase::CanInteract(APawn* InstigatorPawn) const
{
	return !bIsBeingClaimed;
}

void AItemActorBase::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !InstigatorPawn) return;

	UInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return;

	if (!Inventory->TryAddItem(this)) return;

	bIsBeingClaimed = true;
	HoldingPawn = InstigatorPawn;

	UpdateAttachment(); // 서버 자신은 RepNotify가 안 불리니 직접 호출
}

void AItemActorBase::OnRep_HoldingPawn(APawn* OldHoldingPawn)
{
	UpdateAttachment(OldHoldingPawn);
}

void AItemActorBase::UpdateAttachment(APawn* OldHoldingPawn)
{
	if (HoldingPawn)
	{
		// 콜리전은 서버/클라 모두 여기서 꺼야 함 - 서버에서만 끄면 클라에선
		// 계속 켜진 채로 손에 붙어서, 캐릭터 이동 시 자기 콜리전과 부딪혀 떨림.
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// 물리 시뮬레이션은 서버 권위만 유지(클라가 로컬로 물리를 돌리면
		// 서버 리플리케이트 위치랑 따로 놀 수 있음).
		if (HasAuthority())
		{
			MeshComponent->AttachToComponent(Character->GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				SocketProvider->GetRightHandSocketName());

			SocketProvider->SetHoldingItem(true);   // 추가: 캐릭터한테 "들고 있음" 알림
			MeshComponent->SetSimulatePhysics(false);
		}

		if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn))
		{
			Character->AttachItemToRightHand(MeshComponent);
		}
	}
	else
	{
		MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		// 파손 감지(NotifyHit)에 필요한 원래 콜리전으로 복원.
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		if (HasAuthority())
		{
			MeshComponent->SetSimulatePhysics(true);
		}

		if (AGoHomeCharacter* PrevCharacter = Cast<AGoHomeCharacter>(OldHoldingPawn))
		{
			PrevCharacter->DetachItemFromRightHand();
		}
	}
}

float AItemActorBase::GetTotalWeight() const
{
	return ItemData ? ItemData->Weight : 0.f;
}

float AItemActorBase::GetCurrentValue() const
{
	if (!ItemData) return 0.f;

	const float PenaltyRatio = ItemData->BreakValuePenaltyPercent * BreakCount;
	const float ValueRatio = FMath::Max(1.f - PenaltyRatio, ItemData->MinValuePercent);
	return ItemData->Value * ValueRatio;
}



void AItemActorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemActorBase, bIsBeingClaimed);
	DOREPLIFETIME(AItemActorBase, BreakCount);
	DOREPLIFETIME(AItemActorBase, CurrentNoiseRadius);
	DOREPLIFETIME(AItemActorBase, HoldingPawn);
}

void AItemActorBase::NotifyHit(UPrimitiveComponent* MyComp, 
	AActor* Other, 
	UPrimitiveComponent* OtherComp, 
	bool bSelfMoved, 
	FVector HitLocation, 
	FVector HitNormal, 
	FVector NormalImpulse, 
	const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (!HasAuthority() || !ItemData || !ItemData->bCanBreak) return;
	if (BreakCount >= ItemData->MaxBreakCount) return;

	const float ImpactSpeed = GetVelocity().Size();
	if (ImpactSpeed >= ItemData->BreakVelocityThreshold)
	{
		++BreakCount;

		// 테스트용 디버깅 메시지(추후 삭제하면 된다.)
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 5.f, FColor::Red, 
				FString::Printf(TEXT("파손! 속도 =%.0f, BreakCount = %d, 현재가치 = %.0f"), 
					ImpactSpeed, BreakCount, GetCurrentValue()));
		}

	}
}



// NoiseType

void AItemActorBase::NotifyPickedUp()
{
	if (!HasAuthority() || !ItemData || !ItemData->bMakesNoise) return;

	if (CurrentNoiseRadius <= 0.f)
	{
		CurrentNoiseRadius = ItemData->BaseNoiseRadius;
	}

	UGoHomeNoiseLibrary::GenerateNoise(this, GetActorLocation(),
		CurrentNoiseRadius, ENoiseType::Medium, this);

	// 테스트용 디버깅 메시지(추후 삭제할 것).

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("소음 발생! 반경 = %.0f(픽업 직후)"),
			CurrentNoiseRadius));
	}


	// 여기까지 테스트용 디버깅 메시지.

	GetWorldTimerManager().SetTimer(NoiseGrowthTimerHandle, this,
		&AItemActorBase::GrowNoiseRadius, ItemData->NoiseGrowthIntervalSeconds, true);
}

void AItemActorBase::NotifyDropped()
{
	GetWorldTimerManager().ClearTimer(NoiseGrowthTimerHandle);
	// CurrentNoiseRadius는 그대로 둔다 -> 아이템을 드랍한 후 다시 주우면 멈췄던 지점에서 타이머가 이어서 증가.
}

void AItemActorBase::ServerDrop()
{
	if (!HasAuthority() || !HoldingPawn) return;

	APawn* PreviousHolder = HoldingPawn;

	bIsBeingClaimed = false;
	HoldingPawn = nullptr;

	// 서버 자신은 RepNotify 자동 발동 안 되므로 직접 호출.
	UpdateAttachment();

	// 캐릭터한테 "이제 안 들고 있음" 알림 → AnimBP의 HoldAlpha가 내려가면서 원래 Locomotion으로 복귀
	if (ISocketProvider* SocketProvider = Cast<ISocketProvider>(PreviousHolder))
	{
		SocketProvider->SetHoldingItem(false);
	}

	MeshComponent->SetSimulatePhysics(true);
	// 파손 감지(NotifyHit)에 필요한 원래 콜리전으로 복원.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// 서버 자신은 RepNotify 자동 발동 안 되므로 직접 호출(Old holder 넘겨줌).
	UpdateAttachment(PreviousHolder);

	// 제자리에 툭 떨어지지 않게 앞+위 방향으로 던지는 초기 속도 부여.
	const FVector ThrowDirection = (PreviousHolder->GetActorForwardVector() + FVector::UpVector * 0.3f).GetSafeNormal();
	MeshComponent->SetPhysicsLinearVelocity(ThrowDirection * DropThrowSpeed);

	if (UInventoryComponent* Inventory = PreviousHolder->FindComponentByClass<UInventoryComponent>())
	{
		// 내부에서 NotifyDropped() 호출(소음 타이머 정지).
		Inventory->RemoveItem(this);
	}

}


void AItemActorBase::GrowNoiseRadius()
{
	if (!ItemData) return;

	CurrentNoiseRadius = FMath::Min(
		CurrentNoiseRadius + ItemData->NoiseRadiusGrowthPerInterval, 
		ItemData->MaxNoiseRadius);

	const ENoiseType Type = (CurrentNoiseRadius >= 1500.f) ? ENoiseType::Large : ENoiseType::Medium; 
	UGoHomeNoiseLibrary::GenerateNoise(this, GetActorLocation(), CurrentNoiseRadius, Type, this);

	// 테스트용 디버깅 메시지(추후 삭제할 것).
	if (GEngine)
	{
		const TCHAR* TypeStr = (Type == ENoiseType::Large) ? TEXT("Large") : TEXT("Medium");
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("소음 증가! 반경 = %.0f, Type = %s"), CurrentNoiseRadius, TypeStr));
	}
}
