#include "Interaction/SharedLockerActor.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GoHomePlayerController.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ASharedLockerActor::ASharedLockerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// BP 자식에서 상자 메쉬로 바꿔 끼울 수 있는 기본 메쉬.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (DefaultMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultMeshAsset.Object);
	}
}

bool ASharedLockerActor::CanInteract(APawn* InstigatorPawn) const
{
	return InstigatorPawn && Cast<AGoHomePlayerController>(InstigatorPawn->GetController()) != nullptr;
}

void ASharedLockerActor::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !InstigatorPawn)
	{
		return;
	}

	if (AGoHomePlayerController* PlayerController =
		Cast<AGoHomePlayerController>(InstigatorPawn->GetController()))
	{
		PlayerController->Client_OpenSharedLocker();
	}
}

FText ASharedLockerActor::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("보관함"));
}

bool ASharedLockerActor::IsPawnInUseRange(const APawn* Pawn) const
{
	return Pawn && FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(UseRadius);
}
