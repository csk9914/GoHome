#include "Interaction/ShopStation.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GoHomePlayerController.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AShopStation::AShopStation()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// BP 자식에서 상점 터미널 메쉬로 바꿔 끼울 수 있는 기본 메쉬.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (DefaultMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultMeshAsset.Object);
	}
}

bool AShopStation::CanInteract(APawn* InstigatorPawn) const
{
	if (!InstigatorPawn)
	{
		return false;
	}

	return Cast<AGoHomePlayerController>(InstigatorPawn->GetController()) != nullptr;
}

void AShopStation::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !InstigatorPawn)
	{
		return;
	}

	// 다음 단계에서 상호작용한 플레이어에게 상점 UI를 열도록 연결한다.
	if (AGoHomePlayerController* PlayerController =
		Cast<AGoHomePlayerController>(InstigatorPawn->GetController()))
	{
		PlayerController->Client_OpenShop();
	}
}

FText AShopStation::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("상점"));
}