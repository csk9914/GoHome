#include "Interaction/EquipmentUpgradeStation.h"
#include "Core/GoHomePlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/GameInstance.h"
#include "Save/GoHomeSaveSubsystem.h"

AEquipmentUpgradeStation::AEquipmentUpgradeStation()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultMeshAsset.Object);
	}
}

bool AEquipmentUpgradeStation::CanInteract(APawn* InstigatorPawn) const
{
	if (!InstigatorPawn)
	{
		return false;
	}

	return Cast<AGoHomePlayerController>(InstigatorPawn->GetController()) != nullptr;
}

void AEquipmentUpgradeStation::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !InstigatorPawn)
	{
		return;
	}

	if (AGoHomePlayerController* PlayerController =
		Cast<AGoHomePlayerController>(InstigatorPawn->GetController()))
	{
		UGameInstance* GameInstance = GetGameInstance();

		// UI를 연 후 서버의 최신 코인을 보낸다.
		PlayerController->Client_OpenEquipmentUpgrade();

		if (GameInstance)
		{
			if (UGoHomeSaveSubsystem* SaveSubsystem =
				GameInstance->GetSubsystem<UGoHomeSaveSubsystem>())
			{
				// UI를 열기 전에 서버의 최신 코인을 보낸다.
				PlayerController->Client_RefreshUpgradeFunds(
					SaveSubsystem->GetCurrentFunds());
			}
		}
	}
}

FText AEquipmentUpgradeStation::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("강화"));
}