//THE

#include "Interaction/DeliveryPoint.h"
#include "Interaction/InventoryComponent.h"
#include "Components/StaticMeshComponent.h"

ADeliveryPoint::ADeliveryPoint()
{ 
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// 테스트용 임시 자리 표기 메시 -> 나중에 실제 에셋으로 교체.
	static ConstructorHelpers::FObjectFinder<UStaticMesh>DefaultMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (DefaultMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultMeshAsset.Object);
	}
}

bool ADeliveryPoint::CanInteract(APawn* InstigatorPawn) const
{
	if (!InstigatorPawn) return false;

	UInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UInventoryComponent>();
	// 정산 가능한 아이템들이 있어야 상호작용 가능.
	return Inventory && Inventory->GetTotalWeight() > 0.f; 
}

void ADeliveryPoint::OnInteract(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !InstigatorPawn) return;

	if (UInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UInventoryComponent>())
	{
		Inventory->ServerDeliverAllItems();
	}
}

FText ADeliveryPoint::GetInteractionPromptText_Implementation() const
{
	return FText::FromString(TEXT("납품하기"));
}
