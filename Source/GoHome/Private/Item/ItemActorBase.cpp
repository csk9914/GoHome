

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
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

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

	// 새로 주운 아이템을 바로 활성 슬롯으로 전환(손에 부착까지 여기서 처리됨).
	const int32 SlotIndex = Inventory->FindSlotIndexOf(this);
	if (SlotIndex != INDEX_NONE)
	{
		Inventory->SetActiveSlot(SlotIndex);
	}
}

void AItemActorBase::OnRep_HoldingPawn(APawn* OldHoldingPawn)
{
	UpdateAttachment(OldHoldingPawn);
}

void AItemActorBase::UpdateAttachment(APawn* OldHoldingPawn)
{
	if (HoldingPawn && bIsActiveHeld)
	{
		// 활성 슬롯: 손에 보이게 부착.
		MeshComponent->SetVisibility(true, true);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (HasAuthority())
		{
			MeshComponent->SetSimulatePhysics(false);
		}

		if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn))
		{
			Character->AttachItemToRightHand(MeshComponent);
		}
	}
	else if (HoldingPawn && !bIsActiveHeld)
	{
		// 인벤토리엔 있지만 비활성 슬롯: 숨기고 부착 해제.
		MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		MeshComponent->SetVisibility(false, true);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (HasAuthority())
		{
			MeshComponent->SetSimulatePhysics(false);
		}

		if (AGoHomeCharacter* Character = Cast<AGoHomeCharacter>(HoldingPawn))
		{
			Character->DetachItemFromRightHand();
		}
	}
	else
	{
		// 아무도 안 들고 있음(드롭/납품 취소 등): 원래 월드 오브젝트 상태로 복원.
		MeshComponent->SetVisibility(true, true);
		MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
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
	DOREPLIFETIME(AItemActorBase, bIsActiveHeld);
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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("소음 발생! 반경 = %.0f(픽업 직후)"),
			CurrentNoiseRadius));
	}

	GetWorldTimerManager().SetTimer(NoiseGrowthTimerHandle, this,
		&AItemActorBase::GrowNoiseRadius, ItemData->NoiseGrowthIntervalSeconds, true);
}

void AItemActorBase::NotifyDropped()
{
	GetWorldTimerManager().ClearTimer(NoiseGrowthTimerHandle);
}

void AItemActorBase::ServerDrop()
{
	if (!HasAuthority() || !HoldingPawn) return;

	APawn* PreviousHolder = HoldingPawn;

	bIsBeingClaimed = false;
	bIsActiveHeld = false;
	HoldingPawn = nullptr;

	UpdateAttachment(PreviousHolder);

	const FVector ThrowDirection = (PreviousHolder->GetActorForwardVector() + FVector::UpVector * 0.3f).GetSafeNormal();
	MeshComponent->SetPhysicsLinearVelocity(ThrowDirection * DropThrowSpeed);

	if (UInventoryComponent* Inventory = PreviousHolder->FindComponentByClass<UInventoryComponent>())
	{
		Inventory->RemoveItem(this);
	}
}

void AItemActorBase::SetActiveHeld(bool bNewActive)
{
	if (!HasAuthority()) return;
	if (bIsActiveHeld == bNewActive) return;

	bIsActiveHeld = bNewActive;
	UpdateAttachment();
}

void AItemActorBase::OnRep_IsActiveHeld()
{
	UpdateAttachment();
}

void AItemActorBase::GrowNoiseRadius()
{
	if (!ItemData) return;

	CurrentNoiseRadius = FMath::Min(
		CurrentNoiseRadius + ItemData->NoiseRadiusGrowthPerInterval,
		ItemData->MaxNoiseRadius);

	const ENoiseType Type = (CurrentNoiseRadius >= 1500.f) ? ENoiseType::Large : ENoiseType::Medium;
	UGoHomeNoiseLibrary::GenerateNoise(this, GetActorLocation(), CurrentNoiseRadius, Type, this);

	if (GEngine)
	{
		const TCHAR* TypeStr = (Type == ENoiseType::Large) ? TEXT("Large") : TEXT("Medium");
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("소음 증가! 반경 = %.0f, Type = %s"), CurrentNoiseRadius, TypeStr));
	}
}